-- 为角色和场景圣经增加软删除字段
-- 兼容 MySQL 8.0 的幂等迁移：仅当字段不存在时才添加

SET @col_exists := (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'characters'
      AND column_name = 'is_deleted'
);

SET @sql := IF(
    @col_exists = 0,
    'ALTER TABLE characters ADD COLUMN is_deleted TINYINT(1) NOT NULL DEFAULT 0',
    'SELECT 1'
);

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'characters'
      AND column_name = 'deleted_at'
);

SET @sql := IF(
    @col_exists = 0,
    'ALTER TABLE characters ADD COLUMN deleted_at TIMESTAMP NULL',
    'SELECT 1'
);

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'scenes'
      AND column_name = 'is_deleted'
);

SET @sql := IF(
    @col_exists = 0,
    'ALTER TABLE scenes ADD COLUMN is_deleted TINYINT(1) NOT NULL DEFAULT 0',
    'SELECT 1'
);

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'scenes'
      AND column_name = 'deleted_at'
);

SET @sql := IF(
    @col_exists = 0,
    'ALTER TABLE scenes ADD COLUMN deleted_at TIMESTAMP NULL',
    'SELECT 1'
);

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
