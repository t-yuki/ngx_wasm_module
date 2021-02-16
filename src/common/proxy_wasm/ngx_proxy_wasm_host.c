#ifndef DDEBUG
#define DDEBUG 0
#endif
#include "ddebug.h"

#include <ngx_proxy_wasm.h>
#include <ngx_wavm.h>


ngx_int_t
ngx_proxy_wasm_hfuncs_proxy_log(ngx_wavm_instance_t *instance,
    const wasm_val_t args[], wasm_val_t rets[])
{
    ngx_uint_t   level;
    int32_t      log_level, msg_size, msg_data;

    log_level = args[0].of.i32;
    msg_data = args[1].of.i32;
    msg_size = args[2].of.i32;

    switch (log_level) {

    case NGX_PROXY_WASM_LOG_TRACE:
    case NGX_PROXY_WASM_LOG_DEBUG:
        level = NGX_LOG_DEBUG;
        break;

    case NGX_PROXY_WASM_LOG_INFO:
        level = NGX_LOG_INFO;
        break;

    case NGX_PROXY_WASM_LOG_WARNING:
        level = NGX_LOG_WARN;
        break;

    case NGX_PROXY_WASM_LOG_ERROR:
        level = NGX_LOG_ERR;
        break;

    default:
        rets[0] = ngx_proxy_wasm_result_bad_arg();
        return NGX_WAVM_OK;

    }

    ngx_log_error(level, instance->log, 0, "%*s",
                  msg_size, instance->mem_offset + msg_data);

    rets[0] = ngx_proxy_wasm_result_ok();
    return NGX_WAVM_OK;
}


ngx_int_t
ngx_proxy_wasm_hfuncs_nop(ngx_wavm_instance_t *instance,
    const wasm_val_t args[], wasm_val_t rets[])
{
    return NGX_WAVM_OK;
}


static ngx_wavm_host_func_def_t  ngx_proxy_wasm_hfuncs[] = {

   /* 0.1.0 */

   { ngx_string("proxy_set_tick_period_milliseconds"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_get_current_time_nanoseconds"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_get_configuration"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x2,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_get_buffer_bytes"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x5,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_get_header_map_pairs"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x3,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_get_property"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x4,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_set_property"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x4,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_get_shared_data"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x5,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_set_shared_data"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x5,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_register_shared_queue"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x3,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_resolve_shared_queue"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x5,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_dequeue_shared_queue"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x3,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_enqueue_shared_queue"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x3,
     ngx_wavm_arity_i32 },

   /* integration */

   { ngx_string("proxy_abi_version_0_1_0"),
     &ngx_proxy_wasm_hfuncs_nop,
     NULL,
     NULL },

   { ngx_string("proxy_log"),
     &ngx_proxy_wasm_hfuncs_proxy_log,
     ngx_wavm_arity_i32x3,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_http_call"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x10,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_done"),
     &ngx_proxy_wasm_hfuncs_nop,
     NULL,
     ngx_wavm_arity_i32 },

   /* context lifecycle */

   { ngx_string("proxy_set_effective_context"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_context_finalize"),
     &ngx_proxy_wasm_hfuncs_nop,
     NULL,
     ngx_wavm_arity_i32 },

   /* stream */

   { ngx_string("proxy_resume_stream"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_close_stream"),
     NULL,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   /* http */

   { ngx_string("proxy_send_http_response"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x8,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_resume_http_stream"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_close_http_stream"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_close_http_stream"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   /* buffers */

   { ngx_string("proxy_get_buffer"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x5,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_set_buffer"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x5,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_get_map_values"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x5,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_set_map_values"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x5,
     ngx_wavm_arity_i32 },

   /* shared k/v store */

   { ngx_string("proxy_open_shared_kvstore"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x4,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_get_shared_kvstore_key_values"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x6,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_set_shared_kvstore_key_values"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x6,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_add_shared_kvstore_key_values"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x6,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_remove_shared_kvstore_key"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x6,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_delete_shared_kvstore"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x6,
     ngx_wavm_arity_i32 },

   /* shared queue */

   { ngx_string("proxy_open_shared_queue"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x4,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_dequeue_shared_queue_item"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x4,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_enqueue_shared_queue_item"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x4,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_delete_shared_queue"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x4,
     ngx_wavm_arity_i32 },

   /* timers */

   { ngx_string("proxy_create_timer"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x3,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_delete_timer"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   /* stats/metrics */

   { ngx_string("proxy_create_metric"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x4,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_get_metric_value"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x2,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_set_metric_value"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32_i64,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_increment_metric_value"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32_i64,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_delete_metric"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   /* http callouts */

   { ngx_string("proxy_dispatch_http_call"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x10,
     ngx_wavm_arity_i32 },

   /* grpc callouts */

   { ngx_string("proxy_dispatch_grpc_call"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x12,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_open_grpc_stream"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x9,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_send_grpc_stream_message"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x3,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_cancel_grpc_call"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   { ngx_string("proxy_close_grpc_call"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32,
     ngx_wavm_arity_i32 },

   /* custom */

   { ngx_string("proxy_call_custom_function"),
     &ngx_proxy_wasm_hfuncs_nop,
     ngx_wavm_arity_i32x5,
     ngx_wavm_arity_i32 },

   ngx_wavm_hfunc_null
};


ngx_wavm_host_def_t  ngx_proxy_wasm_host = {
    ngx_string("ngx_proxy_wasm"),
    ngx_proxy_wasm_hfuncs,
};