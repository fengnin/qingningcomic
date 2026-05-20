-- 角色肖像版本表：用于增量编辑（qwen-image-edit）后保留历史版本
-- 注意：去掉 FOREIGN KEY 以避免远端 MySQL 链路上 CREATE TABLE DDL 30s 超时；引用完整性由代码层兜底
CREATE TABLE IF NOT EXISTS character_portrait_versions (
    id VARCHAR(36) PRIMARY KEY,
    character_id VARCHAR(36) NOT NULL,
    version_no INT NOT NULL,
    portrait_path VARCHAR(255) NOT NULL,
    base_version_id VARCHAR(36) NULL,
    edit_prompt TEXT NULL,
    field_diff JSON NULL,
    appearance_snapshot JSON NULL,
    source_chapter INT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_character_id (character_id),
    INDEX idx_character_version (character_id, version_no)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 在 characters 表加 current 指针；同步约定：portrait_path 始终镜像 current 版本的 portrait_path
ALTER TABLE characters ADD COLUMN IF NOT EXISTS current_portrait_version_id VARCHAR(36) NULL;
