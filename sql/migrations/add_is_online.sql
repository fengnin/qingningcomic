-- 兼容 MySQL 8.0 的幂等迁移：仅当字段不存在时才添加
SET @col_exists := (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'users'
      AND column_name = 'is_online'
);

SET @sql := IF(
    @col_exists = 0,
    'ALTER TABLE users ADD COLUMN is_online TINYINT(1) NOT NULL DEFAULT 0',
    'SELECT 1'
);

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
