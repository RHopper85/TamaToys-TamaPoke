-- TamaPoke initial MariaDB schema. This script does not delete existing data.
-- Import into the dedicated TamaPoke database (dbs16306000) with phpMyAdmin.

CREATE TABLE IF NOT EXISTS tp_trainers (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    friend_code CHAR(10) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    display_name VARCHAR(10) NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_seen_at DATETIME NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_tp_trainers_friend_code (friend_code)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE IF NOT EXISTS tp_device_tokens (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    trainer_id BIGINT UNSIGNED NOT NULL,
    token_hash CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_used_at DATETIME NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_tp_device_tokens_hash (token_hash),
    KEY idx_tp_device_tokens_trainer (trainer_id),
    CONSTRAINT fk_tp_device_tokens_trainer FOREIGN KEY (trainer_id)
        REFERENCES tp_trainers(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Reserved for the next release. No requests can be sent until authorized
-- endpoints implement acceptance/rejection and enforce both players' wishes.
CREATE TABLE IF NOT EXISTS tp_friend_requests (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    sender_id BIGINT UNSIGNED NOT NULL,
    recipient_id BIGINT UNSIGNED NOT NULL,
    status ENUM('pending', 'accepted', 'rejected') NOT NULL DEFAULT 'pending',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_tp_friend_request_direction (sender_id, recipient_id),
    KEY idx_tp_friend_requests_inbox (recipient_id, status),
    CONSTRAINT fk_tp_friend_requests_sender FOREIGN KEY (sender_id)
        REFERENCES tp_trainers(id) ON DELETE CASCADE,
    CONSTRAINT fk_tp_friend_requests_recipient FOREIGN KEY (recipient_id)
        REFERENCES tp_trainers(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS tp_friendships (
    lower_trainer_id BIGINT UNSIGNED NOT NULL,
    higher_trainer_id BIGINT UNSIGNED NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (lower_trainer_id, higher_trainer_id),
    CONSTRAINT fk_tp_friendships_lower FOREIGN KEY (lower_trainer_id)
        REFERENCES tp_trainers(id) ON DELETE CASCADE,
    CONSTRAINT fk_tp_friendships_higher FOREIGN KEY (higher_trainer_id)
        REFERENCES tp_trainers(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Registration is rate-limited by the server-observed client IP address.
-- IP addresses are hashed before insertion; no plaintext IP is retained.
CREATE TABLE IF NOT EXISTS tp_registration_attempts (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    ip_hash CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_tp_reg_attempts_ip_time (ip_hash, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
