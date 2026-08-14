# ============================================================
# JoyShockMapper GUI 官网 — 纯静态站点 Nginx 镜像
# 来源: template/Dockerfile.vue-nginx (方案A: CI 构建产物传入)
# 构建方式: docker build -f site/Dockerfile site/
# ============================================================
FROM nginx:alpine

# 复制构建好的静态文件 (本仓库 CI 无需前端构建，直接 COPY)
COPY . /usr/share/nginx/html

# 复制 nginx 静态托管配置
COPY nginx.conf /etc/nginx/conf.d/default.conf

# 清理构建产物文件，避免被当作网页暴露
RUN rm -f /usr/share/nginx/html/Dockerfile /usr/share/nginx/html/nginx.conf

EXPOSE 80

HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
  CMD wget --no-verbose --tries=1 --spider http://localhost:80/ || exit 1

CMD ["nginx", "-g", "daemon off;"]
