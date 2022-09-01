# vim:set ft= ts=4 sts=4 sw=4 et fdm=marker:

use strict;
use lib '.';
use t::TestWasm;

skip_valgrind();

plan tests => repeat_each() * (blocks() * 5);

run_tests();

__DATA__

=== TEST 1: shm directive - kv sanity
--- main_config
    wasm {
        shm_kv my_kv_1 1m;
        shm_kv my_kv_2 64k;
    }
--- no_error_log
[error]
[crit]
[emerg]
stub



=== TEST 2: shm directive - queue sanity
--- main_config
    wasm {
        shm_queue my_queue_1 1m;
        shm_queue my_queue_2 64k;
    }
--- no_error_log
[error]
[crit]
[emerg]
stub



=== TEST 3: shm directive - invalid size
--- main_config
    wasm {
        shm_kv my_kv_1 1x;
    }
--- error_log eval
qr/\[emerg\] .*? \[wasm\] invalid shm size "1x"/
--- no_error_log
[error]
[crit]
stub
--- must_die



=== TEST 4: shm directive - too small
--- main_config
    wasm {
        shm_kv my_kv_1 8192;
    }
--- error_log eval
qr/\[emerg\] .*? \[wasm\] shm size of 8192 bytes is too small, minimum required is 12288 bytes/
--- no_error_log
[error]
[crit]
stub
--- must_die



=== TEST 5: shm directive - not aligned
--- main_config
    wasm {
        shm_kv my_kv_1 16383;
    }
--- error_log eval
qr/\[emerg\] .*? \[wasm\] shm size of 16383 bytes is not page-aligned, must be a multiple of 4096/
--- no_error_log
[crit]
[error]
stub
--- must_die



=== TEST 6: shm directive - empty name
--- main_config
    wasm {
        shm_kv "" 16k;
    }
--- error_log eval
qr/\[emerg\] .*? \[wasm\] invalid shm name ""/
--- no_error_log
[error]
[crit]
stub
--- must_die



=== TEST 7: shm directive - duplicate queues
--- main_config
    wasm {
        shm_queue my_queue 16k;
        shm_queue my_queue 16k;
    }
--- error_log eval
qr/\[emerg\] .*? "my_queue" shm already defined/
--- no_error_log
[error]
[crit]
stub
--- must_die



=== TEST 8: shm directive - duplicate kv
--- main_config
    wasm {
        shm_kv my_kv 16k;
        shm_kv my_kv 16k;
    }
--- error_log eval
qr/\[emerg\] .*? "my_kv" shm already defined/
--- no_error_log
[error]
[crit]
stub
--- must_die



=== TEST 9: shm directive - duplicate name between queue and kv
--- main_config
    wasm {
        shm_kv    my_shm 16k;
        shm_queue my_shm 16k;
    }
--- error_log eval
qr/\[emerg\] .*? "my_shm" shm already defined/
--- no_error_log
[error]
[crit]
stub
--- must_die