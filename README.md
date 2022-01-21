ngix-qoi
========

This nginx module converts qoi images on the fly and sends a png response.

qoi image format: https://github.com/phoboslab/qoi


Dependency
----------

[qoi](https://github.com/phoboslab/qoi.git) header.

- qoi.h

[stb](https://github.com/nothings/stb.git) header.

- stb_image.h
- stb_image_write.h


Installation
------------

Build.

``` sh
$ : "clone repository"
$ git clone https://xxx/nginx-qoi
$ cd nginx-qoi
$ : "get nginx source"
$ NGINX_VERSION=1.x.x # specify nginx version
$ wget http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz
$ tar -zxf nginx-${NGINX_VERSION}.tar.gz
$ cd nginx-${NGINX_VERSION}
$ : "build module"
$ ./configure --add-dynamic-module=${PWD}/../
$ make && make install
```

Edit `nginx.conf` and load the required modules.

```
load_module modules/ngx_http_qoi_module.so;
```


Synopsis
--------

``` nginx
http {
  location ~ ".qoi" { qoi; }
}
```


Test
----

Using the perl of [Test::Nginx](https://github.com/openresty/test-nginx) module
to testing.

``` sh
$ cpan -Ti Test::Nginx
$ export TEST_NGINX_HTML_DIR=${PWD}/t/html
$ export TEST_NGINX_LOAD_MODULES=${PWD}/nginx-${NGINX_VERSION}/objs/ngx_http_qoi_module.so
$ prove -r t
t/001.t .. ok
All tests successful.
Files=1, Tests=2,  0 wallclock secs ( 0.01 usr  0.00 sys +  0.11 cusr  0.02 csys =  0.14 CPU)
Result: PASS
```


Example
-------

``` sh
$ curl -I http://localhost/qoi_logo.qoi
HTTP/1.1 200 OK
Server: nginx/1.20.2
Date: Fri, 21 Jan 2022 03:31:56 GMT
Content-Length: 20210
Last-Modified: Fri, 21 Jan 2022 03:31:56 GMT
Connection: keep-alive
Content-Type: image/png
```
