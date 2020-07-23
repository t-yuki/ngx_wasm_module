/*
 * Copyright (C) Thibault Charbonnier
 */

#ifndef DDEBUG
#define DDEBUG 0
#endif
#include "ddebug.h"

#include <ngx_wasm_vm.h>
#include <ngx_wasm_util.h>
#include <ngx_wasm_hostfuncs.h>


#define ngx_wasm_vm_check_init(vm, ret)                                      \
    if (vm->vm_actions == NULL) {                                            \
        ngx_wasm_log_error(NGX_LOG_EMERG, vm->log, 0,                        \
                           "\"%V\" vm not initialized (vm: %p)",             \
                           &vm->name, vm);                                   \
        ngx_wasm_assert(0);                                                  \
        return ret;                                                          \
    }                                                                        \


typedef struct ngx_wasm_vm_module_s  ngx_wasm_vm_module_t;


typedef struct {
    ngx_wasm_vm_t                *vm;
    ngx_wasm_instance_t          *instance;
    ngx_log_t                    *orig_log;
} ngx_wasm_vm_log_ctx_t;


struct ngx_wasm_instance_s {
    ngx_pool_t                   *pool;
    ngx_log_t                    *log;
    ngx_wasm_hctx_t               hctx;
    ngx_wasm_vm_t                *vm;
    ngx_wasm_vm_module_t         *module;
    ngx_wasm_vm_instance_pt       vm_instance;
};


struct ngx_wasm_vm_module_s {
    ngx_rbtree_node_t             rbnode;
    ngx_str_t                     name;
    ngx_str_t                     path;
    ngx_uint_t                    flags;
    ngx_wasm_vm_module_pt         vm_module;
};


typedef struct {
    ngx_rbtree_t                  rbtree;
    ngx_rbtree_node_t             sentinel;
} ngx_wasm_vm_modules_t;


struct ngx_wasm_vm_s {
    ngx_str_t                    name;
    ngx_pool_t                  *pool;
    ngx_log_t                   *log;
    ngx_wasm_vm_log_ctx_t        log_ctx;
    ngx_wasm_hfuncs_store_t     *hf_store;
    ngx_wasm_hfuncs_resolver_t  *hf_resolver;
    ngx_str_t                   *vm_name;
    ngx_wasm_vm_actions_t       *vm_actions;
    ngx_wasm_vm_engine_pt        vm_engine;
    ngx_wasm_vm_modules_t        modules;
};


static void ngx_wasm_vm_log_error(ngx_uint_t level, ngx_log_t *log,
    ngx_wasm_vm_error_pt vm_error, ngx_wasm_vm_trap_pt vm_trap,
#if (NGX_HAVE_VARIADIC_MACROS)
    const char *fmt, ...);

#else
    const char *fmt, va_list args);
#endif
static u_char *ngx_wasm_vm_log_error_handler(ngx_log_t *log, u_char *buf,
    size_t len);


void/*{{{*/
ngx_wasm_vm_modules_insert(ngx_rbtree_node_t *temp,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
    ngx_wasm_vm_module_t   *n, *t;
    ngx_rbtree_node_t     **p;

    for ( ;; ) {

        n = (ngx_wasm_vm_module_t *) node;
        t = (ngx_wasm_vm_module_t *) temp;

        if (node->key != temp->key) {
            p = (node->key < temp->key) ? &temp->left : &temp->right;

        } else if (n->name.len != t->name.len) {
            p = (n->name.len < t->name.len) ? &temp->left : &temp->right;

        } else {
            p = (ngx_memcmp(n->name.data, t->name.data, n->name.len) < 0)
                ? &temp->left : &temp->right;
        }

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}


static ngx_wasm_vm_module_t *
ngx_wasm_vm_modules_lookup(ngx_rbtree_t *rbtree, u_char *name, size_t len,
    uint32_t hash)
{
    ngx_wasm_vm_module_t   *nm;
    ngx_rbtree_node_t      *node, *sentinel;
    ngx_int_t               rc;

    node = rbtree->root;
    sentinel = rbtree->sentinel;

    while (node != sentinel) {
        nm = (ngx_wasm_vm_module_t *) node;

        if (hash != node->key) {
            node = (hash < node->key) ? node->left : node->right;
            continue;
        }

        rc = ngx_memcmp(name, nm->name.data, len);

        if (rc < 0) {
            node = node->left;
            continue;
        }

        if (rc > 0) {
            node = node->right;
            continue;
        }

        return nm;
    }

    return NULL;
}


static ngx_wasm_vm_module_t *
ngx_wasm_vm_get_module(ngx_wasm_vm_t *vm, u_char *name)
{
    uint32_t                      hash;
    size_t                        nlen;

    nlen = ngx_strlen(name);
    hash = ngx_crc32_short(name, nlen);

    return ngx_wasm_vm_modules_lookup(&vm->modules.rbtree, name, nlen, hash);
}/*}}}*/


ngx_wasm_vm_t *
ngx_wasm_vm_new(const char *name, ngx_cycle_t *cycle,
    ngx_wasm_hfuncs_store_t *hf_store)
{
    ngx_wasm_vm_t           *vm;
    u_char                  *p;

    vm = ngx_pcalloc(cycle->pool, sizeof(struct ngx_wasm_vm_s));
    if (vm == NULL) {
        return NULL;
    }

    vm->pool = cycle->pool;
    vm->hf_store = hf_store;

    vm->log_ctx.vm = vm;
    vm->log_ctx.orig_log = &cycle->new_log;

    vm->log = ngx_pcalloc(vm->pool, sizeof(ngx_log_t));
    if (vm->log == NULL) {
        goto failed;
    }

    vm->log->file = cycle->new_log.file;
    vm->log->next = cycle->new_log.next;
    vm->log->writer = cycle->new_log.writer;
    vm->log->wdata = cycle->new_log.wdata;
    vm->log->log_level = cycle->new_log.log_level;
    vm->log->handler = ngx_wasm_vm_log_error_handler;
    vm->log->data = &vm->log_ctx;

    ngx_rbtree_init(&vm->modules.rbtree, &vm->modules.sentinel,
                    ngx_wasm_vm_modules_insert);

    vm->name.len = ngx_strlen(name);
    vm->name.data = ngx_pnalloc(vm->pool, vm->name.len + 1);
    if (vm->name.data == NULL) {
        goto failed;
    }

    p = ngx_copy(vm->name.data, name, vm->name.len);
    *p = '\0';

    ngx_log_debug2(NGX_LOG_DEBUG_WASM, cycle->log, 0,
                   "[wasm] created \"%V\" vm (vm: %p)",
                   &vm->name, vm);

    return vm;

failed:

    ngx_wasm_log_error(NGX_LOG_EMERG, cycle->log, 0,
                       "failed to create \"%*s\" vm", name);

    ngx_wasm_vm_free(vm);

    return NULL;
}


void
ngx_wasm_vm_free(ngx_wasm_vm_t *vm)
{
    ngx_pool_t             *pool;
    ngx_rbtree_node_t     **root, **sentinel;
    ngx_wasm_vm_module_t   *module;

    ngx_log_debug2(NGX_LOG_DEBUG_WASM, vm->log, 0,
                   "[wasm] free \"%V\" vm (vm: %p)",
                   &vm->name, vm);

    pool = vm->pool;

    if (vm->hf_resolver) {
        ngx_wasm_hfuncs_resolver_destroy(vm->hf_resolver);
        ngx_pfree(pool, vm->hf_resolver);
    }

    root = &vm->modules.rbtree.root;
    sentinel = &vm->modules.rbtree.sentinel;

    while (*root != *sentinel) {
        module = (ngx_wasm_vm_module_t *) ngx_rbtree_min(*root, *sentinel);

        ngx_rbtree_delete(&vm->modules.rbtree, &module->rbnode);

        ngx_log_debug4(NGX_LOG_DEBUG_WASM, vm->log, 0,
                       "[wasm] free \"%V\" module in \"%V\" vm"
                       " (vm: %p, module: %p)",
                       &module->name, &vm->name,
                       vm, module);

        vm->vm_actions->module_free(module->vm_module);

        ngx_pfree(pool, module->name.data);
        ngx_pfree(pool, module->path.data);
        ngx_pfree(pool, module);
    }

    if (vm->vm_actions
        && vm->vm_actions->engine_free
        && vm->vm_engine)
    {
        vm->vm_actions->engine_free(vm->vm_engine);
    }

    if (vm->name.data) {
        ngx_pfree(pool, vm->name.data);
    }

    if (vm->log) {
        ngx_pfree(pool, vm->log);
    }

    ngx_pfree(pool, vm);
}


ngx_int_t
ngx_wasm_vm_add_module(ngx_wasm_vm_t *vm, u_char *name, u_char *path)
{
    ngx_wasm_vm_module_t         *module;
    u_char                       *p;
    ngx_rbtree_node_t            *n;

    module = ngx_wasm_vm_get_module(vm, name);
    if (module) {
        return NGX_DECLINED;
    }

    /* module == NULL */

    module = ngx_pcalloc(vm->pool, sizeof(ngx_wasm_vm_module_t));
    if (module == NULL) {
        goto failed;
    }

    module->name.len = ngx_strlen(name);
    module->name.data = ngx_pnalloc(vm->pool, module->name.len + 1);
    if (module->name.data == NULL) {
        goto failed;
    }

    p = ngx_copy(module->name.data, name, module->name.len);
    *p = '\0';

    module->path.len = ngx_strlen(path);
    module->path.data = ngx_pnalloc(vm->pool, module->path.len + 1);
    if (module->path.data == NULL) {
        goto failed;
    }

    p = ngx_copy(module->path.data, path, module->path.len);
    *p = '\0';

    if (ngx_strcmp(&module->path.data[module->path.len - 4], ".wat") == 0) {
        module->flags |= NGX_WASM_VM_MODULE_ISWAT;
    }

    n = &module->rbnode;
    n->key = ngx_crc32_short(module->name.data, module->name.len);
    ngx_rbtree_insert(&vm->modules.rbtree, n);

    ngx_log_debug4(NGX_LOG_DEBUG_WASM, vm->log, 0,
                   "[wasm] registered \"%V\" module in \"%V\" vm"
                   " (vm: %p, module: %p)",
                   &module->name, &vm->name,
                   vm, module);

    return NGX_OK;

failed:

    ngx_wasm_log_error(NGX_LOG_EMERG, vm->log, 0,
                       "failed to add \"%*s\" module from \"%*s\"",
                        name, path);

    if (module) {
        if (module->name.data) {
            ngx_pfree(vm->pool, module->name.data);
        }

        if (module->path.data) {
            ngx_pfree(vm->pool, module->path.data);
        }

        ngx_pfree(vm->pool, module);
    }

    return NGX_ERROR;
}


ngx_int_t
ngx_wasm_vm_has_module(ngx_wasm_vm_t *vm, u_char *mod)
{
    ngx_wasm_vm_module_t         *module;

    module = ngx_wasm_vm_get_module(vm, mod);
    if (module == NULL) {
        return NGX_DECLINED;
    }

    return NGX_OK;
}


ngx_int_t
ngx_wasm_vm_init_runtime(ngx_wasm_vm_t *vm, ngx_str_t *vm_name,
    ngx_wasm_vm_actions_t *vm_actions)
{
    if (vm->vm_actions) {
        return NGX_DONE;
    }

    vm->vm_name = vm_name;
    vm->vm_actions = vm_actions;

    vm->hf_resolver = ngx_pcalloc(vm->pool, sizeof(ngx_wasm_hfuncs_resolver_t));
    if (vm->hf_resolver == NULL) {
        return NGX_ERROR;
    }

    vm->hf_resolver->store = vm->hf_store;
    vm->hf_resolver->pool = vm->pool;
    vm->hf_resolver->log = vm->log;
    vm->hf_resolver->hf_new = vm_actions->hfunc_new;
    vm->hf_resolver->hf_free = vm_actions->hfunc_free;

    if (ngx_wasm_hfuncs_resolver_init(vm->hf_resolver) != NGX_OK) {
        return NGX_ERROR;
    }

    if (vm_actions->engine_new) {
        vm->vm_engine = vm_actions->engine_new(vm->pool, vm->hf_resolver);
        if (vm->vm_engine == NULL) {
            ngx_wasm_log_error(NGX_LOG_EMERG, vm->log, 0,
                               "failed to initialize \"%V\" runtime "
                               "for \"%V\" vm", vm_name, &vm->name);
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}


ngx_int_t
ngx_wasm_vm_load_module(ngx_wasm_vm_t *vm, u_char *mod)
{
    ngx_wasm_vm_module_t  *module;
    ngx_wasm_vm_error_pt   vm_error = NULL;
    ngx_str_t              bytes;
    ngx_fd_t               fd;
    ngx_file_t             file;
    ssize_t                n, fsize;
    ngx_int_t              rc = NGX_ERROR;

    ngx_wasm_vm_check_init(vm, NGX_ABORT);

    module = ngx_wasm_vm_get_module(vm, mod);
    if (module == NULL) {
        ngx_wasm_log_error(NGX_LOG_EMERG, vm->log, 0,
                           "no \"%*s\" module defined", mod);
        return NGX_DECLINED;
    }

    if (module->flags & NGX_WASM_VM_MODULE_LOADED) {
        return NGX_OK;
    }

    if (!module->path.len) {
        ngx_wasm_log_error(NGX_LOG_ALERT, vm->log, 0,
                           "NYI: module loading only supported via path");
        return NGX_ERROR;
    }

    fd = ngx_open_file(module->path.data, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
    if (fd == NGX_INVALID_FILE) {
        ngx_wasm_log_error(NGX_LOG_EMERG, vm->log, ngx_errno,
                           ngx_open_file_n " \"%V\" failed",
                           &module->path);
        return NGX_ERROR;
    }

    ngx_memzero(&file, sizeof(ngx_file_t));

    file.fd = fd;
    file.log = vm->log;
    file.name.len = ngx_strlen(module->path.data);
    file.name.data = (u_char *) module->path.data;

    if (ngx_fd_info(fd, &file.info) == NGX_FILE_ERROR) {
        ngx_wasm_log_error(NGX_LOG_EMERG, vm->log, ngx_errno,
                           ngx_fd_info_n " \"%V\" failed", &file.name);
        goto close;
    }

    fsize = ngx_file_size(&file.info);

    bytes.len = fsize;
    bytes.data = ngx_alloc(bytes.len, vm->log);
    if (bytes.data == NULL) {
        ngx_wasm_log_error(NGX_LOG_EMERG, vm->log, 0,
                           "failed to allocate bytes for \"%V\" "
                           "module from \"%V\"",
                           &module->name, &module->path);
        goto close;
    }

    n = ngx_read_file(&file, (u_char *) bytes.data, fsize, 0);
    if (n == NGX_ERROR) {
        ngx_wasm_log_error(NGX_LOG_EMERG, vm->log, ngx_errno,
                           ngx_read_file_n " \"%V\" failed",
                           &file.name);
        goto close;
    }

    if (n != fsize) {
        ngx_wasm_log_error(NGX_LOG_EMERG, vm->log, 0,
                           ngx_read_file_n " \"%V\" returned only "
                           "%z bytes instead of %uz", &file.name,
                           n, fsize);
        goto close;
    }

    ngx_wasm_log_error(NGX_LOG_NOTICE, vm->log, 0,
                       "loading \"%V\" module from \"%V\"",
                       &module->name, &module->path);

    module->vm_module = vm->vm_actions->module_new(vm->vm_engine,
                                                   &module->name,
                                                   &bytes,
                                                   module->flags,
                                                   &vm_error);
    if (module->vm_module == NULL) {
        ngx_wasm_vm_log_error(NGX_LOG_EMERG, vm->log, vm_error, NULL,
                              "failed to load \"%V\" module from \"%V\"",
                              &module->name, &module->path);
        goto close;
    }

    module->flags |= NGX_WASM_VM_MODULE_LOADED;

    ngx_log_debug4(NGX_LOG_DEBUG_WASM, vm->log, 0,
                   "[wasm] loaded \"%V\" module in \"%V\" vm"
                   " (vm: %p, module: %p)",
                   &module->name, &vm->name,
                   vm, module);

    rc = NGX_OK;

close:

    if (ngx_close_file(fd) == NGX_FILE_ERROR) {
        ngx_wasm_log_error(NGX_LOG_ERR, vm->log, ngx_errno,
                           ngx_close_file_n " \"%V\" failed", &file.name);
    }

    if (bytes.data) {
        ngx_free(bytes.data);
    }

    return rc;
}


ngx_int_t
ngx_wasm_vm_load_modules(ngx_wasm_vm_t *vm)
{
    ngx_wasm_vm_module_t         *module;
    ngx_rbtree_node_t            *node, *root, *sentinel;
    ngx_int_t                     rc;

    ngx_wasm_vm_check_init(vm, NGX_ABORT);

    sentinel = vm->modules.rbtree.sentinel;
    root = vm->modules.rbtree.root;

    if (root == sentinel) {
        return NGX_OK;
    }

    for (node = ngx_rbtree_min(root, sentinel);
         node;
         node = ngx_rbtree_next(&vm->modules.rbtree, node))
    {
        module = (ngx_wasm_vm_module_t *) node;

        rc = ngx_wasm_vm_load_module(vm, module->name.data);
        if (rc != NGX_OK) {
            return rc;
        }
    }

    return NGX_OK;
}


ngx_wasm_instance_t *
ngx_wasm_vm_instance_new(ngx_wasm_vm_t *vm, ngx_str_t *mod,
    ngx_wasm_hctx_t *hctx)
{
    ngx_wasm_instance_t    *instance;
    ngx_wasm_vm_module_t   *module;
    ngx_wasm_vm_log_ctx_t  *log_ctx;
    ngx_wasm_vm_error_pt    vm_error = NULL;
    ngx_wasm_vm_trap_pt     vm_trap = NULL;

    ngx_wasm_vm_check_init(vm, NULL);

    module = ngx_wasm_vm_get_module(vm, mod->data);
    if (module == NULL) {
        ngx_wasm_log_error(NGX_LOG_ERR, vm->log, 0,
                           "failed to create instance of \"%V\" module: "
                           "no such module defined", mod);
        return NULL;
    }

    instance = ngx_palloc(vm->pool, sizeof(struct ngx_wasm_instance_s));
    if (instance == NULL) {
        ngx_wasm_log_error(NGX_LOG_ERR, vm->log, 0,
                           "failed to create instance of \"%V\" module: "
                           "could not allocate memory", mod);
        return NULL;
    }

    instance->vm = vm;
    instance->pool = vm->pool;
    instance->module = module;

    instance->log = ngx_pcalloc(instance->pool, sizeof(ngx_log_t));
    if (instance->log == NULL) {
        goto failed;
    }

    instance->log->file = hctx->log->file;
    instance->log->next = hctx->log->next;
    instance->log->writer = hctx->log->writer;
    instance->log->wdata = hctx->log->wdata;
    instance->log->log_level = hctx->log->log_level;
    instance->log->handler = ngx_wasm_vm_log_error_handler;

    log_ctx = ngx_palloc(instance->pool, sizeof(ngx_wasm_vm_log_ctx_t));
    if (log_ctx == NULL) {
        goto failed;
    }

    log_ctx->vm = vm;
    log_ctx->instance = instance;
    log_ctx->orig_log = hctx->log;

    instance->log->data = log_ctx;

    instance->hctx.log = instance->log;
    instance->hctx.data = hctx->data;
    instance->hctx.memory_offset = NULL;

    ngx_log_debug5(NGX_LOG_DEBUG_WASM, vm->log, 0,
                   "[wasm] creating instance of \"%V\" module in \"%V\" vm"
                   " (vm: %p, module: %p, instance: %p)",
                   &module->name, &vm->name,
                   vm, module, instance);

    instance->vm_instance = vm->vm_actions->instance_new(module->vm_module,
                                                         &instance->hctx,
                                                         &vm_error,
                                                         &vm_trap);
    if (instance->vm_instance == NULL) {
        ngx_wasm_vm_log_error(NGX_LOG_ERR, vm->log, vm_error, vm_trap,
                              "failed to create instance of \"%V\" module",
                              mod);
        goto failed;
    }

    return instance;

failed:

    ngx_wasm_vm_instance_free(instance);

    return NULL;
}


ngx_int_t
ngx_wasm_vm_instance_call(ngx_wasm_instance_t *instance,
    ngx_str_t *func_name)
{
    ngx_wasm_vm_t          *vm = instance->vm;
    ngx_wasm_vm_error_pt    vm_error = NULL;
    ngx_wasm_vm_trap_pt     vm_trap = NULL;
    ngx_int_t               rc;

    ngx_wasm_vm_check_init(vm, NGX_ABORT);

    rc = vm->vm_actions->instance_call(instance->vm_instance, func_name,
                                       &vm_error, &vm_trap);
    if (vm_error || vm_trap) {
        ngx_wasm_vm_log_error(NGX_LOG_ERR, instance->log, vm_error, vm_trap,
                              "func err");
    }

    return rc;
}


void
ngx_wasm_vm_instance_free(ngx_wasm_instance_t *instance)
{
    ngx_log_debug5(NGX_LOG_DEBUG_WASM, instance->log, 0,
                   "[wasm] free instance of \"%V\" module in \"%V\" vm"
                   " (vm: %p, module: %p, instance: %p)",
                   &instance->module->name, &instance->vm->name,
                   instance->vm, instance->module, instance);

    if (instance->vm_instance) {
        instance->vm->vm_actions->instance_free(instance->vm_instance);
    }

    if (instance->log) {
        if (instance->log->data) {
            ngx_pfree(instance->pool, instance->log->data);
        }

        ngx_pfree(instance->pool, instance->log);
    }

    ngx_pfree(instance->pool, instance);
}


static void
ngx_wasm_vm_log_error(ngx_uint_t level, ngx_log_t *log,
    ngx_wasm_vm_error_pt vm_error, ngx_wasm_vm_trap_pt vm_trap,
#if (NGX_HAVE_VARIADIC_MACROS)
    const char *fmt, ...)

#else
    const char *fmt, va_list args)
#endif
{
#if (NGX_HAVE_VARIADIC_MACROS)
    va_list                  args;
#endif
    u_char                  *p, *last;
    u_char                   errstr[NGX_MAX_ERROR_STR];
    ngx_wasm_vm_log_ctx_t   *ctx;
    ngx_wasm_vm_t           *vm;

    ctx = log->data;
    vm = ctx->vm;

    last = errstr + NGX_MAX_ERROR_STR;
    p = &errstr[0];

#if (NGX_HAVE_VARIADIC_MACROS)
    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

#else
    p = ngx_vslprintf(p, last, fmt, args);
#endif

    if (vm->vm_actions == NULL) {
        p = ngx_snprintf(p, last - p, " (\"%V\" vm not initialized)",
                         &vm->name);

    } else if (vm_error || vm_trap) {
        p = vm->vm_actions->log_error_handler(vm_error, vm_trap, p, last - p);
    }

    ngx_wasm_log_error(level, log, 0, "%*s", p - errstr, errstr);
}


static u_char *
ngx_wasm_vm_log_error_handler(ngx_log_t *log, u_char *buf, size_t len)
{
    u_char                  *p = buf;
    ngx_log_t               *orig_log;
    ngx_wasm_vm_log_ctx_t   *ctx;
    ngx_wasm_vm_t           *vm;

    ctx = log->data;
    vm = ctx->vm;
    orig_log = ctx->orig_log;

    if (vm->vm_actions == NULL) {
        p = ngx_snprintf(buf, len, " <vm: %V, runtime: ?>", &vm->name);

    } else {
        p = ngx_snprintf(buf, len, " <vm: %V, runtime: %V>", &vm->name,
                         vm->vm_name);
    }

    len -= p - buf;
    buf = p;

    if (orig_log->handler) {
        p = orig_log->handler(orig_log, buf, len);
    }

    return p;
}