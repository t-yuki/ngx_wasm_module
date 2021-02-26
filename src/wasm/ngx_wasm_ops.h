#ifndef _NGX_WASM_OPS_H_INCLUDED_
#define _NGX_WASM_OPS_H_INCLUDED_


#include <ngx_wavm.h>
#include <ngx_proxy_wasm.h>


typedef struct ngx_wasm_op_s  ngx_wasm_op_t;
typedef struct ngx_wasm_op_ctx_s  ngx_wasm_op_ctx_t;
typedef struct ngx_wasm_ops_engine_s  ngx_wasm_ops_engine_t;


typedef ngx_int_t (*ngx_wasm_op_handler_pt)(ngx_wasm_op_ctx_t *ctx,
    ngx_wasm_phase_t *phase, ngx_wasm_op_t *op);


struct ngx_wasm_op_ctx_s {
    ngx_log_t                               *log;
    ngx_wasm_ops_engine_t                   *ops_engine;
    ngx_wavm_ctx_t                           wv_ctx;
};


typedef enum {
    NGX_WASM_OP_CALL = 1,
    NGX_WASM_OP_PROXY_WASM
} ngx_wasm_op_code_t;


typedef struct {
    ngx_str_t                                func_name;
    ngx_wavm_funcref_t                      *funcref;
} ngx_wasm_op_call_t;


typedef struct {
    ngx_proxy_wasm_module_t                 *pwmodule;
} ngx_wasm_op_proxy_wasm_t;


struct ngx_wasm_op_s {
    ngx_uint_t                               on_phases;
    ngx_wasm_op_code_t                       code;
    ngx_wasm_op_handler_pt                   handler;
    ngx_wavm_host_def_t                     *host;
    ngx_wavm_module_t                       *module;
    ngx_wavm_linked_module_t                *lmodule;

    union {
        ngx_wasm_op_call_t                   call;
        ngx_wasm_op_proxy_wasm_t             proxy_wasm;
    } conf;
};


typedef struct {
    ngx_wasm_phase_t                        *phase;
    ngx_array_t                             *ops;
} ngx_wasm_ops_pipeline_t;


struct ngx_wasm_ops_engine_s {
    ngx_pool_t                              *pool;
    ngx_wavm_t                              *vm;
    ngx_wasm_subsystem_t                    *subsystem;
    ngx_wasm_ops_pipeline_t                **pipelines;
};


ngx_wasm_ops_engine_t *ngx_wasm_ops_engine_new(ngx_pool_t *pool,
    ngx_wavm_t *vm, ngx_wasm_subsystem_t *subsystem);
void ngx_wasm_ops_engine_init(ngx_wasm_ops_engine_t *engine);

ngx_wasm_op_t *ngx_wasm_conf_add_op_call(ngx_conf_t *cf,
    ngx_wasm_ops_engine_t *ops_engine, ngx_wavm_host_def_t *host,
    ngx_str_t *value);
ngx_wasm_op_t *ngx_wasm_conf_add_op_proxy_wasm(ngx_conf_t *cf,
    ngx_wasm_ops_engine_t *ops_engine, ngx_str_t *value);

ngx_int_t ngx_wasm_ops_resume(ngx_wasm_op_ctx_t *ctx, ngx_uint_t phaseidx);


#endif /* _NGX_WASM_OPS_H_INCLUDED_ */
