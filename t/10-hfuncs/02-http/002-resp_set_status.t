# vim:set ft= ts=4 sw=4 et fdm=marker:

use strict;
use lib '.';
use t::TestWasm;

plan tests => repeat_each() * (blocks() * 3);

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

=== TEST 1: resp_set_status: sets status code in 'rewrite' phase
--- SKIP
--- config
    location /t {
        wasm_call rewrite http_tests set_resp_status;
    }
--- response_body
--- no_error_log
[emerg]



=== TEST 2: resp_set_status: sets status code in 'content' phase
--- config
    location /t {
        wasm_call content http_tests set_resp_status;
        wasm_call content http_tests say_hello;
    }
--- error_code: 201
--- response_body
hello say
--- no_error_log
[emerg]



=== TEST 3: resp_set_status: bad usage in 'log' phase
--- config
    location /t {
        return 200;
        wasm_call log http_tests set_resp_status;
    }
--- ignore_response_body
--- error_log eval
qr/\[error\] .*? bad usage: headers already sent/
--- no_error_log
[emerg]
