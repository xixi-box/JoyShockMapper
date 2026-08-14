# JoyShockMapper 官网

JoyShockMapper GUI 的官方介绍页（纯前端静态站）。部署于 `jsm.wangshun.work`。

## 本地预览

```bash
python3 -m http.server 8899
# 打开 http://127.0.0.1:8899
```

## 部署流程

`.github/workflows/deploy-site.yml`：push 到 `master` / `main` / `gui-rework` 且改动
`site/` 目录时，构建 `site/Dockerfile`（nginx 静态托管）并推送到 GHCR + 阿里云 ACR，
再在阿里云 self-hosted runner 上以 `127.0.0.1:8081:80` 运行容器。

### 首次部署前置

1. 阿里云 ECS 需注册带 `aliyun` 标签的 GitHub self-hosted runner。
2. 网关（wangshun-portfolio）中新增路由：`jsm.wangshun.work` → `host.docker.internal:8081`。
3. DNS 中将 `jsm.wangshun.work` 解析到阿里云 ECS 公网 IP。
4. 仓库 Actions 配置：
   - Variables：`ALIYUN_ACR_REGISTRY`、`ALIYUN_ACR_USERNAME`
   - Secrets：`ALIYUN_ACR_PASSWORD`
   - 可选 Variable：`JSM_IMAGE_NAME`（默认 `wangshun_build/joy-shock-mapper`）

## 发布 Release

`.github/workflows/build-portable.yml` 仅在推送 `v*-gui.*` 标签时构建便携版 EXE 并发布
GitHub Release（构建 Windows 便携版 + SHA256 校验和）。
