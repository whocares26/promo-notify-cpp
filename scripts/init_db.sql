CREATE TABLE IF NOT EXISTS campaigns (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    name             TEXT    NOT NULL,
    phone            TEXT    NOT NULL,
    message_template TEXT    NOT NULL,
    recipient_name   TEXT    NOT NULL DEFAULT '',
    status           TEXT    NOT NULL DEFAULT 'created',
    created_at       DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS unsubscribed (
    phone      TEXT PRIMARY KEY,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
