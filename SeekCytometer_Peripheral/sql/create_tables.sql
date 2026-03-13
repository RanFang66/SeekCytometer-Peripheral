-- SeekCytometer Peripheral 参数管理数据库初始化脚本
-- 数据库: PostgreSQL
-- 说明: 用于管理各外设模块的控制参数
-- 使用方法:
--   1. 先创建数据库:  CREATE DATABASE seekcytometer_peripheral;
--   2. 连接数据库:    \c seekcytometer_peripheral
--   3. 执行本脚本:    \i create_tables.sql

-- ============================================================
-- 建表
-- ============================================================

-- 芯片位置表
CREATE TABLE IF NOT EXISTS chip_position (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(64) NOT NULL UNIQUE,
    x_position  INTEGER NOT NULL CHECK (x_position BETWEEN -100000 AND 80000),
    y_position  INTEGER NOT NULL CHECK (y_position BETWEEN -50000 AND 700000)
);

COMMENT ON TABLE  chip_position IS '芯片位置表';
COMMENT ON COLUMN chip_position.name       IS '位置名称';
COMMENT ON COLUMN chip_position.x_position IS 'X轴位置';
COMMENT ON COLUMN chip_position.y_position IS 'Y轴位置';

-- 镜头位置表(Z轴)
CREATE TABLE IF NOT EXISTS lens_position (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(64) NOT NULL UNIQUE,
    z_position  INTEGER NOT NULL CHECK (z_position BETWEEN -10000 AND 35000)
);

COMMENT ON TABLE  lens_position IS '镜头位置表(Z轴)';
COMMENT ON COLUMN lens_position.name       IS '位置名称';
COMMENT ON COLUMN lens_position.z_position IS '镜头(Z)位置';

-- 激光配置表
CREATE TABLE IF NOT EXISTS laser_config (
    id                  SERIAL PRIMARY KEY,
    name                VARCHAR(64) NOT NULL UNIQUE,
    laser_638nm_enable  BOOLEAN NOT NULL DEFAULT FALSE,
    laser_448nm_enable  BOOLEAN NOT NULL DEFAULT FALSE,
    white_led_enable    BOOLEAN NOT NULL DEFAULT FALSE,
    laser_638nm_power   INTEGER NOT NULL DEFAULT 0 CHECK (laser_638nm_power BETWEEN 0 AND 100),
    laser_448nm_power   INTEGER NOT NULL DEFAULT 0 CHECK (laser_448nm_power BETWEEN 0 AND 100),
    white_led_power     INTEGER NOT NULL DEFAULT 0 CHECK (white_led_power BETWEEN 0 AND 100)
);

COMMENT ON TABLE  laser_config IS '激光配置表';
COMMENT ON COLUMN laser_config.name               IS '激光配置名称';
COMMENT ON COLUMN laser_config.laser_638nm_enable  IS '638nm激光使能';
COMMENT ON COLUMN laser_config.laser_448nm_enable  IS '448nm激光使能';
COMMENT ON COLUMN laser_config.white_led_enable    IS '白色LED使能';
COMMENT ON COLUMN laser_config.laser_638nm_power   IS '638nm强度 (0-100)';
COMMENT ON COLUMN laser_config.laser_448nm_power   IS '448nm强度 (0-100)';
COMMENT ON COLUMN laser_config.white_led_power     IS '白色LED强度 (0-100)';

-- 液压控制参数表
CREATE TABLE IF NOT EXISTS hydraulic_control_params (
    id                  SERIAL PRIMARY KEY,
    name                VARCHAR(64) NOT NULL UNIQUE,
    ch1_kp              REAL NOT NULL DEFAULT 2.0,
    ch1_ki              REAL NOT NULL DEFAULT 1.0,
    ch1_feedforward     INTEGER NOT NULL DEFAULT 13000,
    ch2_kp              REAL NOT NULL DEFAULT 2.0,
    ch2_ki              REAL NOT NULL DEFAULT 1.0,
    ch2_feedforward     INTEGER NOT NULL DEFAULT 13000,
    ch3_kp              REAL NOT NULL DEFAULT 2.0,
    ch3_ki              REAL NOT NULL DEFAULT 1.0,
    ch3_feedforward     INTEGER NOT NULL DEFAULT 13000,
    ch4_kp              REAL NOT NULL DEFAULT 2.0,
    ch4_ki              REAL NOT NULL DEFAULT 1.0,
    ch4_feedforward     INTEGER NOT NULL DEFAULT 13000,
    ch5_kp              REAL NOT NULL DEFAULT 2.0,
    ch5_ki              REAL NOT NULL DEFAULT 1.0,
    ch5_feedforward     INTEGER NOT NULL DEFAULT 13000
);

COMMENT ON TABLE  hydraulic_control_params IS '液压控制参数表';
COMMENT ON COLUMN hydraulic_control_params.name            IS '控制参数名称';
COMMENT ON COLUMN hydraulic_control_params.ch1_kp          IS '通道1 Kp';
COMMENT ON COLUMN hydraulic_control_params.ch1_ki          IS '通道1 Ki';
COMMENT ON COLUMN hydraulic_control_params.ch1_feedforward IS '通道1 前馈值';
COMMENT ON COLUMN hydraulic_control_params.ch2_kp          IS '通道2 Kp';
COMMENT ON COLUMN hydraulic_control_params.ch2_ki          IS '通道2 Ki';
COMMENT ON COLUMN hydraulic_control_params.ch2_feedforward IS '通道2 前馈值';
COMMENT ON COLUMN hydraulic_control_params.ch3_kp          IS '通道3 Kp';
COMMENT ON COLUMN hydraulic_control_params.ch3_ki          IS '通道3 Ki';
COMMENT ON COLUMN hydraulic_control_params.ch3_feedforward IS '通道3 前馈值';
COMMENT ON COLUMN hydraulic_control_params.ch4_kp          IS '通道4 Kp';
COMMENT ON COLUMN hydraulic_control_params.ch4_ki          IS '通道4 Ki';
COMMENT ON COLUMN hydraulic_control_params.ch4_feedforward IS '通道4 前馈值';
COMMENT ON COLUMN hydraulic_control_params.ch5_kp          IS '通道5 Kp';
COMMENT ON COLUMN hydraulic_control_params.ch5_ki          IS '通道5 Ki';
COMMENT ON COLUMN hydraulic_control_params.ch5_feedforward IS '通道5 前馈值';

-- ============================================================
-- 插入默认数据 (使用 ON CONFLICT 避免重复插入)
-- ============================================================

-- 芯片位置默认数据
INSERT INTO chip_position (name, x_position, y_position) VALUES
    ('默认位置', 0, 0),
    ('检测区域', 10000, 350000),
    ('进样口',   -5000, 5000)
ON CONFLICT (name) DO NOTHING;

-- 镜头位置默认数据
INSERT INTO lens_position (name, z_position) VALUES
    ('默认位置', 0),
    ('聚焦位置', 15000)
ON CONFLICT (name) DO NOTHING;

-- 激光配置默认数据
INSERT INTO laser_config (name, laser_638nm_enable, laser_448nm_enable, white_led_enable,
                          laser_638nm_power, laser_448nm_power, white_led_power) VALUES
    ('全部关闭',     FALSE, FALSE, FALSE,  0,   0,   0),
    ('638nm激光',    TRUE,  FALSE, FALSE,  50,  0,   0),
    ('448nm激光',    FALSE, TRUE,  FALSE,  0,   50,  0),
    ('双激光',       TRUE,  TRUE,  FALSE,  50,  50,  0),
    ('白光照明',     FALSE, FALSE, TRUE,   0,   0,   60)
ON CONFLICT (name) DO NOTHING;

-- 液压控制参数默认数据
INSERT INTO hydraulic_control_params (name,
    ch1_kp, ch1_ki, ch1_feedforward,
    ch2_kp, ch2_ki, ch2_feedforward,
    ch3_kp, ch3_ki, ch3_feedforward,
    ch4_kp, ch4_ki, ch4_feedforward,
    ch5_kp, ch5_ki, ch5_feedforward) VALUES
    ('默认参数',
     2.0, 1.0, 13000,
     2.0, 1.0, 13000,
     2.0, 1.0, 13000,
     2.0, 1.0, 13000,
     2.0, 1.0, 13000),
    ('高压模式',
     3.0, 1.5, 18000,
     3.0, 1.5, 18000,
     3.0, 1.5, 18000,
     3.0, 1.5, 18000,
     3.0, 1.5, 18000),
    ('低压模式',
     1.5, 0.8, 8000,
     1.5, 0.8, 8000,
     1.5, 0.8, 8000,
     1.5, 0.8, 8000,
     1.5, 0.8, 8000)
ON CONFLICT (name) DO NOTHING;
