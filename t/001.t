use Test::Nginx::Socket 'no_plan';

no_root_location();

run_tests();

__DATA__

=== TEST 1: qoi of response headers
--- config
    root $TEST_NGINX_HTML_DIR;
    location ~ ".qoi" { qoi; }
--- request
    GET /qoi_logo.qoi
--- response_headers
Content-Type: image/png
--- error_code: 200
