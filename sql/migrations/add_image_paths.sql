-- 兼容 MySQL 8.0 的幂等迁移：仅当字段不存在时才添加
SET @col_exists := (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'characters'
      AND column_name = 'portrait_path'
);

SET @sql := IF(
    @col_exists = 0,
    'ALTER TABLE characters ADD COLUMN portrait_path VARCHAR(255)',
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
      AND column_name = 'reference_image_path'
);

SET @sql := IF(
    @col_exists = 0,
    'ALTER TABLE scenes ADD COLUMN reference_image_path VARCHAR(255)',
    'SELECT 1'
);

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
