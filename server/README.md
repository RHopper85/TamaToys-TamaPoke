# TamaPoke API — first deployment

This phase includes device firmware identity integration and Factory Reset
account deletion. The API supplies registration, permanent unique friend codes,
and independent secret tokens. Friend requests and account recovery are not
implemented; there is intentionally no user login or password.

## Layout and privacy

| Repository file | IONOS destination |
| --- | --- |
| `server/public/index.php` | `/tamapoke/index.php` |
| `server/public/.htaccess` | `/tamapoke/.htaccess` |
| `server/_private/app.php` | `/tamapoke/_private/app.php` |
| `server/_private/.htaccess` | `/tamapoke/_private/.htaccess` |
| `server/_private/config.example.php` | **Copy** as `/tamapoke/_private/config.php` and edit only on your computer/host |
| `server/sql/001_initial.sql` | Import in phpMyAdmin; **do not upload to public webspace** |

The subdomain `tamapokeapi.astraobscura.com` is mapped to `/tamapoke`.
Only `index.php` is a public API entry point. Apache must enforce `_private`'s
`Require all denied`; check this before allowing production registrations.

The real `config.php` contains the database password. It is ignored by Git.
**Never push that file, paste its contents into chat, or upload it to a public
repository.** The DB host, name and user are in IONOS > Hosting > Databases.

## Installation using IONOS (web browser + SFTP)

1. In IONOS **Hosting > Databases > Manage**, open **phpMyAdmin** for the
   dedicated TamaPoke database. Select that database and use **Import** to
   import `server/sql/001_initial.sql`. All statements are `CREATE TABLE IF NOT
   EXISTS` — they do not erase existing tables or game data.
2. In your SFTP client, connect to the IONOS webspace and upload files following
   the table above. Confirm hidden `.htaccess` files were uploaded too.
3. Make a private copy of `config.example.php` named `config.php`. Fill its DB
   host, name, username and password from IONOS and upload the copy **only** to
   `/tamapoke/_private/config.php`. Keep the real file out of Git.
4. In a browser, visit:
   - `https://tamapokeapi.astraobscura.com/v1/health`
   - `https://tamapokeapi.astraobscura.com/v1/health/db`
   Each should return JSON with `"status":"ok"`; the second should say
   `"database":"connected"`.
5. Verify `https://tamapokeapi.astraobscura.com/_private/config.example.php`
   returns **403 Forbidden**. If it returns HTTP 200, stop: the private folder
   is not protected and the server must not be used until access is fixed.
6. **VS Code Terminal (PowerShell)**, for a single test registration:

   ```powershell
   $result = Invoke-RestMethod -Method Post -Uri 'https://tamapokeapi.astraobscura.com/v1/register' -ContentType 'application/json' -Body '{"trainer_name":"ROB"}'
   $result.trainer
   ```

   The reply contains `device_token`; do **not** publish it or paste it into
   chat. Future device firmware stores it privately in nonvolatile storage.
   Registration has a limit of five attempts per public IP per hour.

7. **VS Code Terminal (PowerShell)**, to verify that token (same terminal):

   ```powershell
   Invoke-RestMethod -Uri 'https://tamapokeapi.astraobscura.com/v1/me' -Headers @{'X-TamaPoke-Token' = $result.device_token}
   ```

   Some IONOS PHP CGI configurations do not forward the standard Authorization
   header. The custom `X-TamaPoke-Token` header uses the same secret token and
   remains HTTPS-only. The regular `Authorization: Bearer` format still works
   when the server forwards it.

If direct `/v1/...` URLs return 404, try
`https://tamapokeapi.astraobscura.com/index.php?route=/v1/health`. That is the
fallback route when URL rewriting is not available.

## API version 1

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/v1/health` | Confirm PHP is reachable (no DB needed) |
| GET | `/v1/health/db` | Check MariaDB connection (no credentials in result) |
| POST | `/v1/register` | Accept `{"trainer_name":"ROB"}`; return friend code and device token |
| GET | `/v1/me` | Authenticate with `X-TamaPoke-Token: DEVICE_TOKEN` or `Authorization: Bearer DEVICE_TOKEN`; return own trainer |
| DELETE | `/v1/me` | Authenticate; permanently delete own trainer and cascade removal of tokens, friendships and friend requests |

Codes use `TP` followed by eight easy-to-distinguish characters (ten total),
which fits the existing on-device code-entry control. The server protects
uniqueness with a database constraint. A friend code is **not** a login secret.

The server hashes device tokens before storage. No passwords, device tokens or
SQL error messages are exposed in successful health checks or error responses.

On hardware, registration may also send an optional **device-generated** 64-hex
`device_token` in the JSON body. The device saves it before sending so a lost
response can be retried safely: the server looks up the existing token hash and
returns the *same* trainer and friend code. Older clients that omit the field
still receive a server-generated token. The server updates are in
`server/_private/app.php` and `server/public/index.php`; **re-upload both** to
IONOS before running the updated device firmware. No SQL migration is needed.
Check the health URL after upload: it must return `"version":2`. Firmware
checks this before registering, because the previous API would create an
unrecoverable account with a different secret token.

For IONOS's current rewrite issue, the working full paths are
`/index.php?route=/v1/register` and `/index.php?route=/v1/me` (GET and DELETE).
The firmware uses these exact routes. Do not use plain HTTP; the device
verifies TLS with bundled public CA roots and waits for internet time. If IONOS
renews under a different root, update `tamapoke_ca.h` before shipping a firmware
release. Do not disable TLS verification to work around an unknown issuer.

## Still to do before public launch

- Verify firmware token storage, time sync and root CA validation on physical hardware.
- Authenticated friend requests, acceptance, removal, private last-seen.
- Stronger coordinated abuse controls and retention for registration attempts.
- Real-device TLS/certificate verification tests (never disable TLS checking).
- Server-side asynchronous battle lifecycle and team snapshot validation.
