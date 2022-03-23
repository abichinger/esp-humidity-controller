FROM node:16 AS build
COPY package.json /app/

WORKDIR /app
RUN yarn install

COPY . /app
RUN yarn build

FROM nginx:latest
COPY --from=build /app/dist/ /usr/share/nginx/html