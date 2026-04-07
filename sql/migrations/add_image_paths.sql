-- 添加角色肖像路径字段
ALTER TABLE characters ADD COLUMN IF NOT EXISTS portrait_path VARCHAR(255);

-- 添加场景参考图路径字段
ALTER TABLE scenes ADD COLUMN IF NOT EXISTS reference_image_path VARCHAR(255);
