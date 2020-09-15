# vim:set ft= ts=4 sw=4 et fdm=marker:

use strict;
use lib '.';
use t::TestWasm;

plan tests => repeat_each() * (blocks() * 3);

load_nginx_modules("ngx_http_echo_module");

add_block_preprocessor(sub {
    my $block = shift;
    my $main_config = <<_EOC_;
        wasm {
            module http_tests $t::TestWasm::crates/rust_http_tests.wasm;
        }
_EOC_

    if (!defined $block->main_config) {
        $block->set_value("main_config", $main_config);
    }
});

run_tests();

__DATA__

=== TEST 1: wasm_isolation directive: no wasm{} block
--- main_config
--- config
    wasm_isolation main;

    location /t {
        return 200;
    }
--- error_log eval
qr/\[emerg\] .*? no "wasm" section in configuration/
--- must_die



=== TEST 2: wasm_isolation directive: invalid number of arguments
--- config
    wasm_isolation main main;

    location /t {
        return 200;
    }
--- error_log eval
qr/\[emerg\] .*? invalid number of arguments in "wasm_isolation" directive/
--- must_die



=== TEST 3: wasm_isolation directive: invalid mode
--- config
    wasm_isolation none;

    location /t {
        return 200;
    }
--- error_log eval
qr/\[emerg\] .*? invalid isolation mode "none"/
--- must_die



=== TEST 4: wasm_isolation directive: defaults to "main"
--- skip_no_debug: 3
--- config
    location /t {
        wasm_call log http_tests nop;
        return 200;
    }
--- error_log
wasm using "main" isolation mode



=== TEST 5: wasm_isolation 'main': one instance
--- skip_no_debug: 3
--- http_config
    server {
        listen unix:$TEST_NGINX_HTML_DIR/nginx.sock;

        location / {
            wasm_call rewrite http_tests nop;
            echo_flush;
        }
    }
--- config
    wasm_isolation main;

    location /wasm/loc/a {
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/loc/b {
        wasm_call rewrite http_tests nop;
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/server {
        proxy_pass http://unix:$TEST_NGINX_HTML_DIR/nginx.sock;
    }

    location /t {
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/b;
        echo_location /wasm/server;
    }
--- grep_error_log eval
qr/wasm (?:creating instance|calling) .*/
--- grep_error_log_out eval
qr/\Awasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
\z/



=== TEST 6: wasm_isolation 'server': one instance per server
--- skip_no_debug: 3
--- http_config
    wasm_isolation server;

    server {
        listen unix:$TEST_NGINX_HTML_DIR/nginx.sock;

        location / {
            wasm_call rewrite http_tests nop;
            echo_flush;
        }
    }
--- config
    location /wasm/loc/a {
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/loc/b {
        wasm_call rewrite http_tests nop;
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/server {
        proxy_pass http://unix:$TEST_NGINX_HTML_DIR/nginx.sock;
    }

    location /t {
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/b;
        echo_location /wasm/server;
    }
--- grep_error_log eval
qr/wasm (?:creating instance|calling) .*/
--- grep_error_log_out eval
qr/\Awasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
\z/



=== TEST 7: wasm_isolation 'location': one instance per location
--- skip_no_debug: 3
--- http_config
    wasm_isolation location;

    server {
        listen unix:$TEST_NGINX_HTML_DIR/nginx.sock;

        location / {
            wasm_call rewrite http_tests nop;
            echo_flush;
        }
    }
--- config
    location /wasm/loc/a {
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/loc/b {
        wasm_call rewrite http_tests nop;
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/server {
        proxy_pass http://unix:$TEST_NGINX_HTML_DIR/nginx.sock;
    }

    location /t {
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/b;
        echo_location /wasm/server;
    }
--- grep_error_log eval
qr/wasm (?:creating instance|calling) .*/
--- grep_error_log_out eval
qr/\Awasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
\z/



=== TEST 8: wasm_isolation 'request': one instance per request
--- skip_no_debug: 3
--- http_config
    wasm_isolation request;

    server {
        listen unix:$TEST_NGINX_HTML_DIR/nginx.sock;

        location / {
            wasm_call rewrite http_tests nop;
            echo_flush;
        }
    }
--- config
    location /wasm/loc/a {
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/loc/b {
        wasm_call rewrite http_tests nop;
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/server {
        proxy_pass http://unix:$TEST_NGINX_HTML_DIR/nginx.sock;
    }

    location /t {
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/b;
        echo_location /wasm/server;
    }
--- grep_error_log eval
qr/wasm (?:creating instance|calling) .*/
--- grep_error_log_out eval
qr/\Awasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
\z/



=== TEST 9: wasm_isolation 'ephemeral': one instance per call
--- skip_no_debug: 3
--- http_config
    wasm_isolation ephemeral;

    server {
        listen unix:$TEST_NGINX_HTML_DIR/nginx.sock;

        location / {
            wasm_call rewrite http_tests nop;
            echo_flush;
        }
    }
--- config
    location /wasm/loc/a {
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/loc/b {
        wasm_call rewrite http_tests nop;
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/server {
        proxy_pass http://unix:$TEST_NGINX_HTML_DIR/nginx.sock;
    }

    location /t {
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/b;
        echo_location /wasm/server;
    }
--- grep_error_log eval
qr/wasm (?:creating instance|calling) .*/
--- grep_error_log_out eval
qr/\Awasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
\z/



=== TEST 10: wasm_isolation: nested (1/2)
--- skip_no_debug: 3
--- http_config
    wasm_isolation server;

    server {
        listen         unix:$TEST_NGINX_HTML_DIR/nginx.sock;
        wasm_isolation server;

        location / {
            wasm_call rewrite http_tests nop;
            echo_flush;
        }
    }
--- config
    location /wasm/loc/a {
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/loc/b {
        wasm_isolation ephemeral;
        wasm_call rewrite http_tests nop;
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/server {
        proxy_pass http://unix:$TEST_NGINX_HTML_DIR/nginx.sock;
    }

    location /t {
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/b;
        echo_location /wasm/server;
    }
--- grep_error_log eval
qr/wasm (?:creating instance|calling) .*/
--- grep_error_log_out eval
qr/\Awasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
\z/



=== TEST 11: wasm_isolation: nested (2/2)
--- skip_no_debug: 3
--- http_config
    wasm_isolation location;

    server {
        listen         unix:$TEST_NGINX_HTML_DIR/nginx.sock;
        wasm_isolation main;

        location / {
            wasm_call rewrite http_tests nop;
            echo_flush;
        }
    }
--- config
    location /wasm/loc/a {
        wasm_isolation request;
        wasm_call      rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/loc/b {
        wasm_isolation main;
        wasm_call rewrite http_tests nop;
        wasm_call rewrite http_tests nop;
        echo_flush;
    }

    location /wasm/server {
        proxy_pass http://unix:$TEST_NGINX_HTML_DIR/nginx.sock;
    }

    location /t {
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/a;
        echo_location /wasm/loc/b;
        echo_location /wasm/server;
    }
--- grep_error_log eval
qr/wasm (?:creating instance|calling) .*/
--- grep_error_log_out eval
qr/\Awasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm creating instance of \S+ module in \S+ vm .*?
wasm calling \S+ in "rewrite" phase
wasm calling \S+ in "rewrite" phase
\z/