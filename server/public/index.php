<?php
declare(strict_types=1);

require __DIR__ . '/_private/app.php';

try {
    $method = strtoupper($_SERVER['REQUEST_METHOD'] ?? 'GET');
    $requestedPath = $_GET['route'] ?? parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH);
    $route = '/' . ltrim((string) $requestedPath, '/');

    if ($method === 'GET' && ($route === '/v1/health' || $route === '/index.php')) {
        tpJson(200, ['status' => 'ok', 'service' => 'TamaPoke API', 'version' => 2]);
    }
    if ($method === 'GET' && $route === '/v1/health/db') {
        tpDatabase()->query('SELECT 1');
        tpJson(200, ['status' => 'ok', 'database' => 'connected']);
    }
    if ($method === 'POST' && $route === '/v1/register') {
        tpRequireHttps();
        tpRegister(tpDatabase());
    }
    if ($method === 'GET' && $route === '/v1/me') {
        tpRequireHttps();
        tpCurrentTrainer(tpDatabase());
    }
    if ($method === 'DELETE' && $route === '/v1/me') {
        tpRequireHttps();
        tpDeleteTrainer(tpDatabase());
    }
    tpJson(404, ['error' => 'not_found', 'message' => 'Unknown API endpoint']);
} catch (TpApiError $exception) {
    tpJson($exception->httpStatus, [
        'error' => $exception->errorCode,
        'message' => $exception->getMessage(),
    ]);
} catch (Throwable $exception) {
    // No SQL queries, stack traces, database credentials or device tokens are
    // ever returned to clients. Logs show the error type only.
    error_log('TamaPoke API internal error: ' . get_class($exception));
    tpJson(500, ['error' => 'server_error', 'message' => 'Unexpected server error']);
}
