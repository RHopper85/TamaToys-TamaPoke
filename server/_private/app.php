<?php
declare(strict_types=1);

// TamaPoke API v1. Keep this file and config.php behind the public web root.

final class TpApiError extends RuntimeException
{
    public function __construct(
        public readonly int $httpStatus,
        public readonly string $errorCode,
        string $message
    ) {
        parent::__construct($message);
    }
}

function tpJson(int $status, array $data): never
{
    http_response_code($status);
    header('Content-Type: application/json; charset=utf-8');
    header('Cache-Control: no-store');
    header('X-Content-Type-Options: nosniff');
    echo json_encode($data, JSON_UNESCAPED_SLASHES | JSON_INVALID_UTF8_SUBSTITUTE);
    exit;
}

function tpDatabase(): PDO
{
    static $connection = null;
    if ($connection instanceof PDO) {
        return $connection;
    }

    $configPath = __DIR__ . '/config.php';
    if (!is_file($configPath)) {
        throw new TpApiError(503, 'not_configured', 'Server configuration is missing');
    }
    $config = require $configPath;
    if (!is_array($config)) {
        throw new TpApiError(503, 'not_configured', 'Invalid server configuration');
    }
    foreach (['db_host', 'db_name', 'db_user', 'db_password'] as $key) {
        if (!isset($config[$key]) || !is_string($config[$key]) || $config[$key] === '' ||
            str_starts_with($config[$key], 'REPLACE_WITH_')) {
            throw new TpApiError(503, 'not_configured', 'Database connection is not configured');
        }
    }

    $host = $config['db_host'];
    $name = $config['db_name'];
    $port = (int) ($config['db_port'] ?? 3306);
    if (preg_match('/[;\x00-\x1F]/', $host . $name) || $port < 1 || $port > 65535) {
        throw new TpApiError(503, 'not_configured', 'Database connection is not configured');
    }

    try {
        $connection = new PDO(
            "mysql:host={$host};port={$port};dbname={$name};charset=utf8mb4",
            $config['db_user'],
            $config['db_password'],
            [
                PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
                PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
                PDO::ATTR_EMULATE_PREPARES => false,
                PDO::ATTR_TIMEOUT => 5,
            ]
        );
        $connection->exec("SET time_zone = '+00:00'");
        return $connection;
    } catch (PDOException $exception) {
        error_log('TamaPoke database unavailable: ' . $exception->getCode());
        throw new TpApiError(503, 'database_unavailable', 'The database is temporarily unavailable');
    }
}

function tpRequestJson(): array
{
    $contentType = strtolower(trim(explode(';', $_SERVER['CONTENT_TYPE'] ?? '')[0]));
    if ($contentType !== 'application/json') {
        throw new TpApiError(415, 'expected_json', 'Send Content-Type: application/json');
    }

    if ((int) ($_SERVER['CONTENT_LENGTH'] ?? 0) > 4096) {
        throw new TpApiError(413, 'too_large', 'Request is too large');
    }
    $raw = file_get_contents('php://input', false, null, 0, 4097);
    if (!is_string($raw) || strlen($raw) > 4096) {
        throw new TpApiError(413, 'too_large', 'Request is too large');
    }
    $json = json_decode($raw);
    if (!($json instanceof stdClass)) {
        throw new TpApiError(400, 'invalid_json', 'Expected a JSON object');
    }
    return (array) $json;
}

function tpRequireHttps(): void
{
    $https = $_SERVER['HTTPS'] ?? '';
    if ($https === 'on' || $https === '1' || (int) ($_SERVER['SERVER_PORT'] ?? 0) === 443) {
        return;
    }
    throw new TpApiError(426, 'https_required', 'Use an HTTPS connection');
}

function tpTrainerName(mixed $input): string
{
    if (!is_string($input)) {
        throw new TpApiError(422, 'invalid_name', 'Enter a trainer name');
    }
    $name = strtoupper(trim($input));
    if (!preg_match('/^[A-Z0-9 .-]{1,10}$/D', $name)) {
        throw new TpApiError(422, 'invalid_name', 'Use 1-10 letters, numbers, spaces, dots or dashes');
    }
    return $name;
}

function tpFriendCode(): string
{
    // 32 easily distinguished symbols; eight random symbols = 40 bits.
    // The TP prefix is part of the code and fits the 12-char on-device input.
    $alphabet = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
    $random = random_bytes(8);
    $code = 'TP';
    for ($i = 0; $i < 8; $i++) {
        $code .= $alphabet[ord($random[$i]) & 31];
    }
    return $code;
}

function tpRegister(PDO $db): never
{
    $payload = tpRequestJson();
    $name = tpTrainerName($payload['trainer_name'] ?? null);

    // Firmware generates and stores a random secret BEFORE the first POST.
    // A lost HTTP response can be retried without creating an orphan account.
    // Older API clients that omit this field still receive a server token.
    $suppliedToken = $payload['device_token'] ?? null;
    if ($suppliedToken !== null &&
        (!is_string($suppliedToken) || !preg_match('/^[a-f0-9]{64}$/D', $suppliedToken))) {
        throw new TpApiError(422, 'invalid_token', 'Device token format is invalid');
    }
    if ($suppliedToken !== null) {
        $existing = tpLookupToken($db, $suppliedToken);
        if ($existing !== null) {
            tpJson(200, [
                'trainer' => ['name' => $existing['display_name'], 'friend_code' => $existing['friend_code']],
                'device_token' => $suppliedToken,
            ]);
        }
    }

    // Never trust client-provided X-Forwarded-For headers on shared hosting.
    $ipHash = hash('sha256', $_SERVER['REMOTE_ADDR'] ?? 'unknown');
    $count = $db->prepare(
        'SELECT COUNT(*) FROM tp_registration_attempts '
        . 'WHERE ip_hash = ? AND created_at > UTC_TIMESTAMP() - INTERVAL 1 HOUR'
    );
    $count->execute([$ipHash]);
    if ((int) $count->fetchColumn() >= 5) {
        throw new TpApiError(429, 'rate_limited', 'Try registration again later');
    }

    // The token is a device credential, NOT a friend code. Only its SHA-256
    // digest is stored in MariaDB. Deliver the token once over HTTPS.
    for ($tries = 0; $tries < 5; $tries++) {
        $friendCode = tpFriendCode();
        $deviceToken = $suppliedToken ?? bin2hex(random_bytes(32));
        try {
            $db->beginTransaction();
            $attempt = $db->prepare('INSERT INTO tp_registration_attempts (ip_hash) VALUES (?)');
            $attempt->execute([$ipHash]);

            $trainer = $db->prepare('INSERT INTO tp_trainers (friend_code, display_name) VALUES (?, ?)');
            $trainer->execute([$friendCode, $name]);
            $trainerId = (int) $db->lastInsertId();

            $token = $db->prepare('INSERT INTO tp_device_tokens (trainer_id, token_hash) VALUES (?, ?)');
            $token->execute([$trainerId, hash('sha256', $deviceToken)]);
            $db->commit();

            tpJson(201, [
                'trainer' => ['name' => $name, 'friend_code' => $friendCode],
                'device_token' => $deviceToken,
            ]);
        } catch (PDOException $exception) {
            if ($db->inTransaction()) {
                $db->rollBack();
            }
            if ($exception->getCode() === '23000') {
                // Concurrent retry with the same secret; return the first
                // registration instead of creating a second account.
                if ($suppliedToken !== null && ($existing = tpLookupToken($db, $suppliedToken))) {
                    tpJson(200, [
                        'trainer' => ['name' => $existing['display_name'], 'friend_code' => $existing['friend_code']],
                        'device_token' => $suppliedToken,
                    ]);
                }
                continue; // Extremely unlikely friend-code or token collision.
            }
            throw $exception;
        }
    }
    throw new TpApiError(503, 'registration_retry', 'Please try registration again');
}

function tpLookupToken(PDO $db, string $deviceToken): ?array
{
    $query = $db->prepare(
        'SELECT t.id, t.friend_code, t.display_name FROM tp_device_tokens AS d '
        . 'JOIN tp_trainers AS t ON t.id = d.trainer_id WHERE d.token_hash = ?'
    );
    $query->execute([hash('sha256', strtolower($deviceToken))]);
    $result = $query->fetch();
    return $result ?: null;
}

function tpAuthenticatedTrainer(PDO $db): array
{
    $authorization = $_SERVER['HTTP_AUTHORIZATION'] ?? $_SERVER['REDIRECT_HTTP_AUTHORIZATION'] ?? '';
    // Some shared-hosting CGI gateways do not pass Authorization to PHP.
    // Also accept an explicit private header over HTTPS; never accept tokens
    // in a URL or query string, where access logs could expose them.
    if ($authorization === '' && isset($_SERVER['HTTP_X_TAMAPOKE_TOKEN'])) {
        $authorization = 'Bearer ' . trim((string) $_SERVER['HTTP_X_TAMAPOKE_TOKEN']);
    }
    if (!preg_match('/^Bearer ([a-f0-9]{64})$/iD', $authorization, $matches)) {
        throw new TpApiError(401, 'unauthorized', 'A device token is required');
    }
    $hash = hash('sha256', strtolower($matches[1]));
    $trainer = tpLookupToken($db, $matches[1]);
    if (!$trainer) {
        throw new TpApiError(401, 'unauthorized', 'Device token is invalid');
    }

    $touch = $db->prepare(
        'UPDATE tp_trainers SET last_seen_at = UTC_TIMESTAMP() WHERE id = ?'
    );
    $touch->execute([$trainer['id']]);
    $tokenTouch = $db->prepare(
        'UPDATE tp_device_tokens SET last_used_at = UTC_TIMESTAMP() WHERE token_hash = ?'
    );
    $tokenTouch->execute([$hash]);
    return $trainer;
}

function tpCurrentTrainer(PDO $db): never
{
    $trainer = tpAuthenticatedTrainer($db);
    tpJson(200, [
        'trainer' => ['name' => $trainer['display_name'], 'friend_code' => $trainer['friend_code']],
    ]);
}

function tpDeleteTrainer(PDO $db): never
{
    $trainer = tpAuthenticatedTrainer($db);
    // Foreign-key ON DELETE CASCADE removes device tokens, outstanding friend
    // requests (sent and received), and both directions of friendships.
    $remove = $db->prepare('DELETE FROM tp_trainers WHERE id = ?');
    $remove->execute([$trainer['id']]);
    tpJson(200, ['status' => 'deleted']);
}
