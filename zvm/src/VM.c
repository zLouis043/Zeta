#include "VM.h"
#include "common/Log.h"

#define PANIC_SHOW_STACK(panic_loc, ...) \
    if(vm->has_dbg_info){                \
    log_(RUNTIME_ERROR, panic_loc, __VA_ARGS__); \
    if(vm->codeDA.items[vm->ip].is_extended){   \
    for(size_t m = 0; m < vm->codeDA.items[vm->ip].exd_da_ext.count; m++){ \
    log_(INFO, &vm->codeDA.items[vm->ip].exd_da_ext.items[m], "Macro expanded here");                                          \
    }                                        \
    }                                     \
    }else{                               \
    log_(RUNTIME_ERROR, NULL, __VA_ARGS__); \
    }                                    \
    if(vm->call_stack.count > 0){        \
    size_t count = vm->call_stack.count;                                 \
        for(size_t q = 0; q < count; q++){                                    \
            StackSlot z;                 \
            if(!pop(&vm->call_stack, &z)){             \
                PANIC(RUNTIME_ERROR, NULL, "Stack underflow");                             \
            }                            \
            if(vm->has_dbg_info){                \
            log_(INFO, &vm->codeDA.items[(int)z.as.short_+1].loc, "Called here");            \
            }\
        } \
    }                                     \
    vm->exit_value = -1;                 \
    return

void execute_op(VM * vm, Op op);
Op get_op(VM * vm);

void vm_init(VM *vm, char * filepath, Configs conf){
    vm->conf = conf;
    vm->org_filepath = SV(filepath);
    vm->flags = 0;
    vm->ip = 0;
    vm->has_dbg_info = false;
}

#include <sys/time.h>
struct timespec start, end;

static int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p){
    return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

void vm_gather(VM * vm){

    if(vm->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    vm->stream = fopen(vm->org_filepath.data, "rb");

    if(!vm->stream){
        PANIC(GENERIC_ERROR, NULL, "Could not open file: %*.s\n", vm->org_filepath.len, vm->org_filepath.data);
    }

    char TAG[6]; char DATA_TAG[11]; char CODE_TAG[11];
    size_t mem_cap, stack_cap;
    size_t data_chunk_size, code_chunk_size;
    size_t entry_point;

    fread(&TAG, sizeof(char), 5, vm->stream);
    fread(&vm->has_dbg_info, sizeof(char),1,vm->stream);

    if(vm->has_dbg_info){
        size_t count;
        fread(&count, sizeof(size_t), 1, vm->stream);
        char ** files;
        files = (char**)malloc(count * sizeof(char*));
        for(size_t i = 0; i < count; i++){
            size_t len;
            fread(&len, sizeof(size_t), 1, vm->stream);
            files[i] = malloc(len+1);
            fread(files[i], sizeof(char), len, vm->stream);
            files[i][len] = 0;
            da_push(vm->fda, files[i]);
        }
    }

    fread(&mem_cap, sizeof(size_t), 1, vm->stream);
    fread(&stack_cap, sizeof(size_t), 1, vm->stream);

    stack_init(&vm->runtime_stack, stack_cap);
    stack_init(&vm->call_stack, 64);
    mem_init(&vm->memory, mem_cap);

    fread(&DATA_TAG, sizeof(char), 10, vm->stream);
    fread(&data_chunk_size, sizeof(size_t), 1, vm->stream);

    vm->data_chunk_size = data_chunk_size;

    for(size_t i = 0; i < data_chunk_size; i++){
        Op op = get_op(vm);
        da_push(vm->dataDA, op);
    }

    fread(&CODE_TAG, sizeof(char), 10, vm->stream);
    fread(&code_chunk_size, sizeof(size_t), 1, vm->stream);
    fread(&entry_point, sizeof(size_t), 1, vm->stream);

    vm->code_chunk_size = code_chunk_size;

    if(code_chunk_size <= 1){
        PANIC(GENERIC_ERROR, NULL, "Code segment in file %.*s is empty", vm->org_filepath.len, vm->org_filepath.data);
    }

    vm->entry_point = entry_point;

    for(size_t i = 0; i < code_chunk_size; i++){
        Op op = get_op(vm);
        da_push(vm->codeDA, op);
    }


    fclose(vm->stream);

    if(vm->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        log_(INFO, NULL, "File parsing took %lf seconds.",
                (timespecDiff(&end, &start) * 0.000000001));
    }
}

int vm_run(VM * vm){

    if(vm->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    for(size_t i = 0; i < 8; i++){
        vm->regs[i].value.char_ = 0;
        vm->regs[i].value.short_ = 0;
        vm->regs[i].value.int_ = 0;
        vm->regs[i].value.double_ = 0;
        vm->regs[i].value.ptr_ = 0;
        vm->regs[i].type = INTEGER_;
    }

    for(size_t i = 0; i < vm->data_chunk_size; i++){
        Op op = vm->dataDA.items[i];
        execute_op(vm, op);
        if(vm->exit_value != 0) break;
    }

    vm->end_program = false;
    vm->ip = vm->entry_point;
    for(; vm->ip < vm->code_chunk_size; vm->ip++){
        Op op = vm->codeDA.items[vm->ip];
        execute_op(vm, op);
        if(vm->exit_value != 0) break;
        if(vm->end_program) break;
    }

    if(vm->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        log_(INFO, NULL, "Execution took %lf seconds.", (timespecDiff(&end, &start) * 0.000000001));
    }

    return vm->exit_value;
}

Op op(u8 opcode, As val, Type type){
    return (Op){
            .opcode = opcode,
            .as = val,
            .type = type,
            .is_extended = false,
    };
}

Op op_loc(u8 opcode, As val, Type type, Location loc){
    return (Op){
            .opcode = opcode,
            .as = val,
            .type = type,
            .is_extended = false,
            .loc = loc
    };
}

Op op_ext(u8 opcode, As val, Type type, LocDA ext){
    return (Op){
            .opcode = opcode,
            .as = val,
            .type = type,
            .is_extended = true,
            .exd_da_ext = ext
    };
}

Op op_loc_ext(u8 opcode, As val, Type type, Location loc, LocDA ext){
    return (Op){
            .opcode = opcode,
            .as = val,
            .type = type,
            .loc = loc,
            .is_extended = true,
            .exd_da_ext = ext
    };
}

Op get_op(VM * vm){
    u8 opcode, type;
    u8 idx = 0, is_extended = 0, idx_ext = 0;
    short line = 0, col = 0, line_ext = 0, col_ext = 0;
    size_t ext_count = 0;
    OpInfoDA opida = {0};
    fread(&opcode, sizeof(u8), 1, vm->stream);
    if(vm->has_dbg_info){
        fread(&is_extended, sizeof(u8), 1, vm->stream);
        if(is_extended){
            fread(&ext_count, sizeof(size_t), 1, vm->stream);
            for(size_t i = 0; i < ext_count; i++) {
                fread(&idx_ext, sizeof(u8), 1, vm->stream);
                fread(&line_ext, sizeof(short), 1, vm->stream);
                fread(&col_ext, sizeof(short), 1, vm->stream);
                OpInfo opi = {
                        .file_idx = idx_ext,
                        .line = line_ext,
                        .col = col_ext
                };
                da_push(opida, opi);
            }
        }
        fread(&idx, sizeof(u8), 1, vm->stream);
        fread(&line, sizeof(short), 1, vm->stream);
        fread(&col, sizeof(short), 1, vm->stream);
    }
    fread(&type, sizeof(u8), 1, vm->stream);
    As val;
    switch(type){
        case CHAR_:
            fread(&val.one, sizeof(u8),1,vm->stream);
            break;
        case PTR_:
            fread(&val.two, sizeof(short ),1,vm->stream);
            break;
        case INTEGER_:
            fread(&val.four, sizeof(int ),1,vm->stream);
            break;
        case DOUBLE_:
            fread(&val.eight, sizeof(double ),1,vm->stream);
            break;
        case BYTE_:
            fread(&val.one, sizeof(u8),1,vm->stream);
            break;
        case SHORT_:
            fread(&val.two, sizeof(short ),1,vm->stream);
            break;
        case BYTE_ARRAY_: {
            size_t count = 0;
            fread(&count, sizeof(size_t), 1, vm->stream);
            val.bda.items = malloc(count);
            val.bda.capacity = count;
            val.bda.count = 0;
            for (size_t i = 0; i < count; i++) {
                char c;
                fread(&c, sizeof(char), 1, vm->stream);
                val.bda.items[val.bda.count++] = (u8)c;
            }
        }break;
        case NONE_:
            break;
    }
    if(vm->has_dbg_info){
        Location loc = {
                .filepath = SV(vm->fda.items[idx]),
                .line = line,
                .column = col
        };
        if(is_extended){
            LocDA lda = {0};
            for(size_t i =0; i< ext_count; i++) {
                Location ext = {
                        .filepath = SV(vm->fda.items[opida.items[i].file_idx]),
                        .line = opida.items[i].line,
                        .column = opida.items[i].col
                };
                da_push(lda, ext);
            }
            return op_loc_ext(opcode, val, type, loc, lda);
        }else {
            return op_loc(opcode, val, type, loc);
        }
    }else{
        if(is_extended){
            LocDA lda = {0};
            for(size_t i =0; i< ext_count; i++) {
                Location ext = {
                        .filepath = SV(vm->fda.items[opida.items[i].file_idx]),
                        .line = opida.items[i].line,
                        .column = opida.items[i].col
                };
                da_push(lda, ext);
            }
            return op_ext(opcode, val, type, lda);
        }else {
            return op(opcode, val, type);
        }
    }
}

#define POP(stack, where) \
    if(!pop(&vm->stack, &where)){\
    PANIC_SHOW_STACK(&op.loc, "Stack underflow");\
    }

#define ADD_SUB_OP(operation, type_of_a, as_type) \
    StackSlot a, b;\
    POP(runtime_stack, a);\
    if(a.type != type_of_a){\
    PANIC_SHOW_STACK(&op.loc, "Expected char as the second operand of addC operation.");\
    }\
    POP(runtime_stack, b);\
    switch(b.type){\
    case CHAR_:\
    push(&vm->runtime_stack, stackSlot(as_char_(b.as.char_ operation a.as.as_type), CHAR_));\
    break;\
    case IN_PTR_:\
    push(&vm->runtime_stack, stackSlot(as_short_(b.as.short_ operation a.as.as_type), IN_PTR_));\
    break;\
    case INTEGER_:\
    push(&vm->runtime_stack, stackSlot(as_int_(b.as.int_ operation a.as.as_type), INTEGER_));\
    break;\
    case PTR_:                                       \
    push(&vm->runtime_stack, stackSlot(as_ptr_(b.as.ptr_ operation a.as.as_type), PTR_));   \
    break;\
    case DOUBLE_:\
    push(&vm->runtime_stack, stackSlot(as_double_(b.as.double_ operation a.as.as_type), DOUBLE_));\
    break;                                        \
    default:break;\
    }

#define MUL_DIV_OP(operation, type_of_a, as_type) \
    StackSlot a, b;\
    POP(runtime_stack, a);\
    if(a.type != type_of_a){\
    PANIC_SHOW_STACK(&op.loc, "Expected char as the second operand of addC operation.");\
    }\
    POP(runtime_stack, b);\
    switch(b.type){\
    case CHAR_:\
    push(&vm->runtime_stack, stackSlot(as_char_(b.as.char_ operation a.as.as_type), CHAR_));\
    break;\
    case IN_PTR_:\
    push(&vm->runtime_stack, stackSlot(as_short_(b.as.short_ operation a.as.as_type), IN_PTR_));\
    break;\
    case INTEGER_:\
    push(&vm->runtime_stack, stackSlot(as_int_(b.as.int_ operation a.as.as_type), INTEGER_));\
    break;\
    case PTR_:                                       \
    PANIC_SHOW_STACK(&op.loc, "Cannot multiply or divide a pointer by any value.");\
    break;\
    case DOUBLE_:\
    push(&vm->runtime_stack, stackSlot(as_double_(b.as.double_ operation a.as.as_type), DOUBLE_));\
    break;                                        \
    default: break;                                              \
    }

#define CAST(as_func, TYPE) \
    switch(to_cast.type) {\
    case CHAR_:\
    push(&vm->runtime_stack, stackSlot(as_func(to_cast.as.char_), TYPE));\
    break;\
    case IN_PTR_:\
    push(&vm->runtime_stack, stackSlot(as_func(to_cast.as.short_), TYPE));\
    break;\
    case INTEGER_:\
    push(&vm->runtime_stack, stackSlot(as_func(to_cast.as.int_), TYPE));\
    break;\
    case PTR_:\
    push(&vm->runtime_stack, stackSlot(as_func((uintptr_t)to_cast.as.ptr_), TYPE));\
    break;\
    case DOUBLE_:\
    push(&vm->runtime_stack, stackSlot(as_func(to_cast.as.double_), TYPE));\
    break;                  \
    default:break;                        \
    }

#define SETFLAGS(as_type, cmp_val, cmp_type) \
    StackSlot a;\
    if(!pop(&vm->runtime_stack, &a)){\
    PANIC_SHOW_STACK( &op.loc, "Stack underflow.");\
    }                     \
    if(a.type != cmp_type){         \
    PANIC_SHOW_STACK(&op.loc, "Expected same type to compare. Given type: %s. Maybe try to cast the stack top to the type: %s", type_to_str(a.type), type_to_str(cmp_type));                            \
    }                      \
    if(a.as.as_type == cmp_val){\
    vm->flags &=  ~6;\
    vm->flags |= 8;\
    }else if(a.as.as_type > cmp_val){\
    vm->flags &=  ~10;\
    vm->flags |= 4;\
    }else{\
    vm->flags &=  ~12;\
    vm->flags |= 2;\
    }

#define JMP_IF(x, jmp_val)\
    if(x){       \
    if(jmp_val < 0){   \
         PANIC_SHOW_STACK(&op.loc, "Illegal operation address to jump: %zu", x);\
    }                     \
    vm->ip = jmp_val;     \
    }

#define LOGICAL_OP(operator) \
    StackSlot to_and, value;\
    POP(runtime_stack, value);\
    POP(runtime_stack, to_and);\
    if (to_and.type == PTR_ || to_and.type == DOUBLE_) {\
    PANIC_SHOW_STACK(&op.loc, "Cannot use a pointer or a double for a logical operation. Found: %s",\
                     type_to_str(to_and.type));\
    }\
    if (to_and.type == PTR_ || to_and.type == DOUBLE_) {\
    PANIC_SHOW_STACK(&op.loc, "Cannot use a logical operator on a pointer or a double. Found: %s",\
                     type_to_str(to_and.type));\
    }\
    int and_val;\
    switch (value.type) {\
    case CHAR_: {\
    and_val = value.as.char_;\
    }break;\
    case IN_PTR_: {\
    and_val = value.as.short_;\
    }break;\
    case INTEGER_: {\
    and_val = value.as.int_;\
    }break;\
    default:break;                         \
    }\
    switch (to_and.type) {\
    case CHAR_: {\
    push(&vm->runtime_stack, stackSlot(as_char_(to_and.as.char_ operator and_val), CHAR_));\
    }break;\
    case IN_PTR_: {\
    push(&vm->runtime_stack, stackSlot(as_short_(to_and.as.short_ operator and_val), IN_PTR_));\
    }break;\
    case INTEGER_: {\
    push(&vm->runtime_stack, stackSlot(as_int_(to_and.as.int_ operator and_val), INTEGER_));\
    }break;                  \
    default: break;                         \
    }

#define as_char_(value) ((AsSlot){.char_ = value})
#define as_short_(value) ((AsSlot){.short_ = value})
#define as_int_(value) ((AsSlot){.int_ = value})
#define as_ptr_(value) ((AsSlot){.ptr_ = value})
#define as_double_(value) ((AsSlot){.double_ = value})

#include <unistd.h>

void internal_call(VM * vm, int n_args, int id){

    Stack internal_stack = {0};
    stack_init(&internal_stack, n_args);

    for(int i = 0; i < n_args; i++){
        StackSlot a;

        if(!pop(&vm->runtime_stack, &a)){
            PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack underflow.");
            return;
        }

        if(!push(&internal_stack, a)){\
            PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack overflow.");
            return;
        }
    }

    switch(id){
        case 0:{

            StackSlot max_char_count, dst_buff, fd;

            if(!pop(&internal_stack, &fd)){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack underflow.");
                return;
            }

            if(fd.type != INTEGER_){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Expected integer for file descriptor in read internal call.");
                return;
            }

            if(!pop(&internal_stack, &dst_buff)){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack underflow.");
                return;
            }

            if(dst_buff.type != PTR_){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Expected pointer for destination buffer in read internal call.");
                return;
            }

            if(!pop(&internal_stack, &max_char_count)){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack underflow.");
                return;
            }

            if(max_char_count.type != INTEGER_){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Expected integer for max char count in read internal call.");
                return;
            }

            read(fd.as.int_, dst_buff.as.ptr_, max_char_count.as.int_);
        }break;
        case 1:{

            StackSlot max_char_count, src_buff, fd;

            if(!pop(&internal_stack, &fd)){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack underflow.");
                return;
            }

            if(fd.type != INTEGER_){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Expected integer for file descriptor in write internal call. Instead it found: %s",
                                 type_to_str(fd.type));
                return;
            }


            if(!pop(&internal_stack, &src_buff)){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack underflow.");
                return;
            }

            if(src_buff.type != PTR_){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Expected pointer for source buffer in write internal call. Instead it found: %s",
                                 type_to_str(src_buff.type));
                return;
            }

            if(!pop(&internal_stack, &max_char_count)){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack underflow.");
                return;
            }

            if(max_char_count.type != INTEGER_){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Expected integer for max char count in write internal call. Instead it found: %s",
                                 type_to_str(max_char_count.type));
                return;
            }

            write(fd.as.int_, src_buff.as.ptr_ , max_char_count.as.int_);
        }break;
        case 2:{ // open
        }break;
        case 3:{ // close
        }break;
        case 4:{ // alloc
            StackSlot n_bytes;

            if(!pop(&internal_stack, &n_bytes)){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack underflow.");
                return;
            }

            if(n_bytes.type != INTEGER_){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Expected integer as number of bytes to allocate for alloc syscall.");
                return;
            }

            char *ptr = malloc(n_bytes.as.int_);
            push(&vm->runtime_stack, stackSlot(as_ptr_(ptr), PTR_));
        }break;
        case 5:{ // free
            StackSlot ptr;

            if(!pop(&internal_stack, &ptr)){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Stack underflow.");
                return;
            }

            if(ptr.type != PTR_){
                PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Expected ptr to free for free syscall.");
                return;
            }
            free(ptr.as.ptr_);
            push(&vm->runtime_stack, stackSlot(as_ptr_(ptr.as.ptr_), PTR_));
        }break;
        default:{
            PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Unknown id for internal operation: %d.", id);
            return;
        }
    }
    if(internal_stack.count > 0){
        PANIC_SHOW_STACK(&vm->codeDA.items[vm->ip].loc, "Exceeding values on the stack for internal call %d", id);
        return;
    }
}

void execute_op(VM * vm, Op op){
    switch(op.opcode) {
        case OP_PUSH_C: {
            push(&vm->runtime_stack, stackSlot(as_char_(op.as.one), CHAR_));
        }break;
        case OP_PUSH_I: {
            push(&vm->runtime_stack, stackSlot(as_int_(op.as.four), INTEGER_));
        }break;
        case OP_PUSH_D: {
            push(&vm->runtime_stack, stackSlot(as_double_(op.as.eight), DOUBLE_));
        }break;
        case OP_PUSH_PR: {
            int ptr = op.as.two;
            MemorySlot ms = vm->memory.mem[ptr];
            Type t = ms.type;
            switch(t){
                case CHAR_:{
                    push(&vm->runtime_stack, stackSlot(as_char_(ms.byte), CHAR_));
                }break;
                case INTEGER_:{
                    int bytes = 0;
                    for(int j = 0; j < 4; j++){
                        bytes = vm->memory.mem[ptr+j].byte << (8 * j) | bytes;
                    }
                    push(&vm->runtime_stack, stackSlot(as_int_(bytes), INTEGER_));
                }break;
                case PTR_:{
                    u8 bytes[8];
                    for(int j = 0; j < 8; j++){
                        bytes[j] = vm->memory.mem[ptr+j].byte;
                    }
                    char* val = from_u8(char*, bytes);
                    push(&vm->runtime_stack, stackSlot(as_ptr_(val), PTR_));
                }break;
                case DOUBLE_:{
                    u8 bytes[8];
                    for(int j = 0; j < 8; j++){
                        bytes[j] = vm->memory.mem[ptr+j].byte;
                    }
                    double val = from_u8(double, bytes);
                    push(&vm->runtime_stack, stackSlot(as_double_(val), DOUBLE_));
                }break;
                default: break;
            }
        }break;
        case OP_PUSH_P: {
            push(&vm->runtime_stack, stackSlot(as_short_(op.as.two), IN_PTR_));
        }break;
        case OP_PUSH_STR: {
            char * str = (char*)malloc(op.as.bda.count * sizeof(char));
            for(int i = 0; i < op.as.bda.count; i++){
                str[i] = op.as.bda.items[i];
            }
            push(&vm->runtime_stack, stackSlot(as_ptr_(str), PTR_));
        }break;
        case OP_ADD_C: {
            ADD_SUB_OP(+, CHAR_ , char_);
        }break;
        case OP_SUB_C: {
            ADD_SUB_OP(-, CHAR_ , char_);
        }break;
        case OP_MUL_C: {
            MUL_DIV_OP(*, CHAR_ , char_);
        }break;
        case OP_DIV_C:{
            MUL_DIV_OP(/, CHAR_ , char_);
        }break;
        case OP_ADD_I: {
            ADD_SUB_OP(+, INTEGER_ , int_);
        }break;
        case OP_SUB_I: {
            ADD_SUB_OP(-, INTEGER_ , int_);
        }break;
        case OP_MUL_I: {
            MUL_DIV_OP(*, INTEGER_ , int_);
        }break;
        case OP_DIV_I:{
            MUL_DIV_OP(/, INTEGER_ , int_);
        }break;
        case OP_ADD_D: {
            StackSlot a, b;
            POP(runtime_stack, a);
            if(a.type != DOUBLE_){
                PANIC_SHOW_STACK(&op.loc, "Expected char as the second operand of addC operation.");
            }
            POP(runtime_stack, b);
            switch(b.type){
                case CHAR_:
                    push(&vm->runtime_stack, stackSlot(as_char_(b.as.char_ + a.as.double_), CHAR_));
                    break;
                case IN_PTR_:
                    push(&vm->runtime_stack, stackSlot(as_short_(b.as.short_ + a.as.double_), IN_PTR_));
                    break;
                case INTEGER_:
                    push(&vm->runtime_stack, stackSlot(as_int_(b.as.int_ + a.as.double_), INTEGER_));
                    break;
                case PTR_:
                    PANIC_SHOW_STACK(&op.loc, "Cannot increase a pointer by a double.");
                    break;
                case DOUBLE_:
                    push(&vm->runtime_stack, stackSlot(as_double_(b.as.double_ + a.as.double_), DOUBLE_));
                    break;
                default: break;
            }
        }break;
        case OP_SUB_D: {
            StackSlot a, b;
            POP(runtime_stack, a);
            if(a.type != DOUBLE_){
                PANIC_SHOW_STACK(&op.loc, "Expected char as the second operand of addC operation.");
            }
            POP(runtime_stack, b);
            switch(b.type){
                case CHAR_:
                    push(&vm->runtime_stack, stackSlot(as_char_(b.as.char_ - a.as.double_), CHAR_));
                    break;
                case IN_PTR_:
                    push(&vm->runtime_stack, stackSlot(as_short_(b.as.short_ - a.as.double_), IN_PTR_));
                    break;
                case INTEGER_:
                    push(&vm->runtime_stack, stackSlot(as_int_(b.as.int_ - a.as.double_), INTEGER_));
                    break;
                case PTR_:
                    PANIC_SHOW_STACK(&op.loc, "Cannot decrease a pointer by a double.");
                    break;
                case DOUBLE_:
                    push(&vm->runtime_stack, stackSlot(as_double_(b.as.double_ - a.as.double_), DOUBLE_));
                    break;
                default: break;
            }
        }break;
        case OP_MUL_D: {
            MUL_DIV_OP(*, DOUBLE_ , double_);
        }break;
        case OP_DIV_D:{
            MUL_DIV_OP(/, DOUBLE_ , double_);
        }break;
        case OP_DUP: {
            if (vm->runtime_stack.count < 1) {
                PANIC_SHOW_STACK(&op.loc, "Stack underflow");
            }
            StackSlot ss = vm->runtime_stack.slots[vm->runtime_stack.count - 1 - op.as.two];
            push(&vm->runtime_stack, ss);
        }break;
        case OP_CONST_CHAR_DEF: {
            mem_append(&vm->memory, memSlot(op.as.one, CHAR_, true));
        }break;
        case OP_CONST_INTEGER_DEF: {
            u8 bytes[4];
            for(size_t j = 0; j < 4; j++){
                bytes[j] = (op.as.four >> 8 * j) & 0xFF;
            }
            for(size_t j =0; j< 4; j++){
                mem_append(&vm->memory, memSlot(bytes[j], INTEGER_, true));
            }
        }break;
        case OP_CONST_DOUBLE_DEF: {
            u8 * bytes = to_u8(op.as.eight);
            for(size_t j =0; j< 8; j++){
                mem_append(&vm->memory, memSlot(bytes[j], DOUBLE_, true));
            }
        }break;
        case OP_STRING_DEF:{
            char * str = (char*)malloc(op.as.bda.count * sizeof(char));
            for (size_t j = 0; j < op.as.bda.count; j++) {
                str[j] = op.as.bda.items[j];
            }
            u8 bytes[8];
            for(size_t j = 0; j < 8; j++){
                bytes[j] = ((uintptr_t)str >> 8 * j) & 0xFF;
            }
            for(int j =0; j< 8; j++){
                mem_append(&vm->memory, memSlot(bytes[j], PTR_, true));
            }
        }break;
        case OP_MEM_DEF: {
            for (size_t j = 0; j < op.as.four; j++) {
                mem_append(&vm->memory, memSlot(0, BYTE_, false));
            }
        }break;
        case OP_CMP_C:{
            SETFLAGS(char_, op.as.one, CHAR_);
        }break;
        case OP_CMP_I:{
            SETFLAGS(int_, op.as.four, INTEGER_);
        }break;
        case OP_CMP_PR:{
            int ptr = op.as.two;
            MemorySlot ms = vm->memory.mem[ptr];

            switch(ms.type){
                case CHAR_:{
                    SETFLAGS(char_, ms.byte, CHAR_);
                }break;
                case INTEGER_:{
                    int bytes = 0;
                    for(int j = 0; j < 4; j++){
                        bytes = vm->memory.mem[ptr+j].byte << (8 * j) | bytes;
                    }
                    SETFLAGS(int_, bytes, INTEGER_);
                }break;
                case PTR_:{
                    u8 bytes[8];
                    for(int j = 0; j < 8; j++){
                        bytes[j] = vm->memory.mem[ptr+j].byte;
                    }
                    char* val = from_u8(char*, bytes);
                    SETFLAGS(ptr_, val, PTR_);
                }break;
                case DOUBLE_:{
                    u8 bytes[8];
                    for(int j = 0; j < 8; j++){
                        bytes[j] = vm->memory.mem[ptr+j].byte;
                    }
                    double val = from_u8(double, bytes);
                    SETFLAGS(double_, val, DOUBLE_);
                }break;
                default: break;
            }
        }break;
        case OP_CMP_D:{
            SETFLAGS(double_, op.as.eight, DOUBLE_);
        }break;
        case OP_CALL:{
            int jmp = vm->ip - 1;
            push(&vm->call_stack, stackSlot(as_short_(jmp), IN_PTR_));
            JMP_IF(1, op.as.two);
        }break;
        case OP_RET:{
            StackSlot a;

            if(!pop(&vm->call_stack, &a)){
                PANIC_SHOW_STACK(&op.loc, "No address on the call stack to return to.");
            }

            if(a.type != IN_PTR_){
                PANIC_SHOW_STACK(&op.loc, "[Unexpected behaviour] Expected return address on the call stack. Instead it found: %s",
                                 type_to_str(a.type));
            }
            JMP_IF(1, a.as.short_ + 1);
        }break;
        case OP_JMP:{
            JMP_IF(1, op.as.two);
        }break;
        case OP_JMP_Z:{
            JMP_IF(vm->flags & 1, op.as.two);
        }break;
        case OP_JMP_NZ:{
            JMP_IF(!(vm->flags & 1), op.as.two);
        }break;
        case OP_JMP_E:{
            JMP_IF(vm->flags & 8, op.as.two);
        }break;
        case OP_JMP_NE:{
            JMP_IF(!(vm->flags & 8), op.as.two);
        }break;
        case OP_JMP_G:{
            JMP_IF(vm->flags & 4, op.as.two);
        }break;
        case OP_JMP_GE:{
            JMP_IF((vm->flags & 4) || (vm->flags & 8), op.as.two);
        }break;
        case OP_JMP_L:{
            JMP_IF(vm->flags & 2, op.as.two);
        }break;
        case OP_JMP_LE:{
            JMP_IF((vm->flags & 2) || (vm->flags & 8), op.as.two);
        }break;
        case OP_DEREF: {
            StackSlot ss;
            POP(runtime_stack, ss);
            Type t = ss.type;
            switch(t) {
                case IN_PTR_: {
                    short ptr = ss.as.short_;
                    MemorySlot ms = vm->memory.mem[ptr];
                    switch (ms.type) {
                        case BYTE_:{
                            push(&vm->runtime_stack, stackSlot(as_char_(ms.byte), INTEGER_));
                        }break;
                        case CHAR_: {
                            push(&vm->runtime_stack, stackSlot(as_char_(ms.byte), CHAR_));
                        }break;
                        case IN_PTR_: {
                            short bytes = 0;
                            for(int j = 0; j < 2; j++){
                                bytes = vm->memory.mem[ptr+j].byte << (8 * j) | bytes;//(bytes >> 8 * j) & 0xFF;
                            }
                            push(&vm->runtime_stack, stackSlot(as_short_(bytes), IN_PTR_));
                        }break;
                        case INTEGER_: {
                            int bytes = 0;
                            for(int j = 0; j < 4; j++){
                                bytes = vm->memory.mem[ptr+j].byte << (8 * j) | bytes;
                            }
                            push(&vm->runtime_stack, stackSlot(as_int_(bytes), INTEGER_));
                        }break;
                        case DOUBLE_: {
                            u8 bytes[8];
                            for(int j = 0; j < 8; j++){
                                bytes[j] = vm->memory.mem[ptr+j].byte;
                            }
                            double val = from_u8(double, bytes);
                            push(&vm->runtime_stack, stackSlot(as_double_(val), DOUBLE_));
                        }break;
                        case PTR_: {
                            u8 bytes[8];
                            for(int j = 0; j < 8; j++){
                                bytes[j] = vm->memory.mem[ptr+j].byte;
                            }
                            char* val = from_u8(char*, bytes);
                            push(&vm->runtime_stack, stackSlot(as_ptr_(val), PTR_));
                        }break;
                        default: break;
                    }
                }break;
                case PTR_: {
                    char *p_value = ss.as.ptr_;
                    push(&vm->runtime_stack, stackSlot(as_char_(*p_value), CHAR_));
                }break;
                default:
                    PANIC_SHOW_STACK(&op.loc, "Invalid type to deref: %s", type_to_str(t));
                    break;
            }
        }break;
        case OP_MOVE: {
            StackSlot to_move;
            POP(runtime_stack, to_move);

            vm->regs[op.as.one].value = to_move.as;
            vm->regs[op.as.one].type = to_move.type;
        }break;
        case OP_GET: {
            push(&vm->runtime_stack, stackSlot(vm->regs[op.as.one].value, vm->regs[op.as.one].type));
        }break;
        case OP_CLEAN:
            vm->regs[op.as.one].value.char_ = 0;
            vm->regs[op.as.one].value.short_ = 0;
            vm->regs[op.as.one].value.int_ = 0;
            vm->regs[op.as.one].value.double_ = 0;
            vm->regs[op.as.one].value.ptr_ = 0;
            vm->regs[op.as.one].type = INTEGER_;
            break;
        case OP_CAST: {
            StackSlot to_cast;
            POP(runtime_stack, to_cast);
            int type_cast = op.as.one;

            switch(type_cast){
                case 0: {
                    CAST(as_char_, CHAR_);
                }break;
                case 1: {
                    CAST(as_short_, IN_PTR_);
                }break;
                case 2: {
                    CAST(as_int_, INTEGER_);
                }break;
                case 3: {
                    CAST(as_double_, DOUBLE_);
                }break;
                case 4:{
                    if(to_cast.type != INTEGER_){
                        PANIC_SHOW_STACK(&op.loc, "Only an integer can be casted to a pointer.");
                    }
                    push(&vm->runtime_stack, stackSlot(as_ptr_((char*)((uintptr_t)to_cast.as.int_)), PTR_));
                }
            }

        }break;
        case OP_DROP: {
            StackSlot to_drop;
            POP(runtime_stack, to_drop);
        }break;
        case OP_DUMP: {

            StackSlot to_dump;

            POP(runtime_stack, to_dump);

            AsSlot value = to_dump.as;
            Type t = to_dump.type;
            if(t == DOUBLE_){
                printf("%0.13g", value.double_);
            }else if(t == INTEGER_){
                printf("%d", value.int_);
            }else if(t == CHAR_){
                printf("%c", value.char_);
            }else if(t == IN_PTR_){
                printf("0x%hd", value.short_);
            }else if(t == PTR_){
                printf("0x%p", value.ptr_);
            }
        }break;
        case OP_EXIT: {
            StackSlot a;

            POP(runtime_stack, a);

            switch(a.type){
                case CHAR_:{
                    vm->exit_value = a.as.char_;
                }break;
                case INTEGER_:{
                    vm->exit_value = a.as.int_;
                }break;
                case IN_PTR_:
                case PTR_:
                case DOUBLE_:{
                    PANIC_SHOW_STACK(&op.loc, "Expected integer for the exit value");
                }break;
                default: break;
            }
            vm->end_program = true;
        }break;
        case OP_SWAP: {
            StackSlot a,b;

            POP(runtime_stack, a);
            POP(runtime_stack, b);

            push(&vm->runtime_stack, a);
            push(&vm->runtime_stack, b);
        }break;
        case OP_STORE: {
            StackSlot ptr, value;
            POP(runtime_stack, value);
            POP(runtime_stack, ptr);

            if(ptr.type != IN_PTR_ && ptr.type != PTR_){
                PANIC_SHOW_STACK(&op.loc, "Expected a heap pointer or a static memory pointer on the stack as the first operand of the store operation. Instead found: %s",
                                 type_to_str(ptr.type));
            }


            Type t = value.type;
            switch(ptr.type) {
                case IN_PTR_: {
                    switch (t) {
                        case CHAR_: {
                            vm->memory.mem[ptr.as.short_].byte = value.as.char_;
                            vm->memory.mem[ptr.as.short_].type = t;
                        }break;
                        case IN_PTR_: {
                            u8 bytes[2];
                            for(int j = 0; j < 2; j++){
                                bytes[j] = (value.as.short_ >> 8 * j) & 0xFF;
                            }
                            for(int j =0; j< 2; j++){
                                vm->memory.mem[ptr.as.short_ + j].byte = bytes[j];
                                vm->memory.mem[ptr.as.short_].type = t;
                            }
                        }break;
                        case INTEGER_: {
                            u8 bytes[4];
                            for(int j = 0; j < 4; j++){
                                bytes[j] = (value.as.int_ >> 8 * j) & 0xFF;
                            }
                            for(int j =0; j< 4; j++){
                                vm->memory.mem[ptr.as.short_ + j].byte = bytes[j];
                                vm->memory.mem[ptr.as.short_].type = t;
                            }
                        }break;
                        case DOUBLE_: {
                            u8 * bytes = to_u8(value.as.double_);
                            for(int j =0; j< 8; j++){
                                vm->memory.mem[ptr.as.short_ + j].byte = bytes[j];
                                vm->memory.mem[ptr.as.short_].type = t;
                            }
                        }break;
                        case PTR_: {
                            u8 * bytes = to_u8(value.as.ptr_);
                            for(int j =0; j< 8; j++){
                                vm->memory.mem[ptr.as.short_ + j].byte = bytes[j];
                                vm->memory.mem[ptr.as.short_].type = t;
                            }
                        }break;
                        default: break;
                    }
                }break;
                case PTR_: {
                    switch (t) {
                        case CHAR_: {
                            char *ptr_c = ptr.as.ptr_;
                            ptr_c[0] = value.as.char_;
                        }break;
                        case IN_PTR_: {
                            short *ptr_c = ptr.as.ptr_;
                            u8 bytes[2];
                            for(int j = 0; j < 2; j++){
                                bytes[j] = (value.as.short_ >> 8 * j) & 0xFF;
                            }
                            for(int j =0; j< 2; j++){
                                ptr_c[j] = bytes[j];
                            }
                        }break;
                        case INTEGER_: {
                            int *ptr_c = ptr.as.ptr_;
                            u8 bytes[4];
                            for(int j = 0; j < 4; j++){
                                bytes[j] = (value.as.int_ >> 8 * j) & 0xFF;
                            }
                            for(int j =0; j< 4; j++){
                                ptr_c[j] = bytes[j];
                            }
                        }break;
                        case DOUBLE_: {
                            double *ptr_c = ptr.as.ptr_;
                            u8 * bytes = to_u8(value.as.double_);
                            for(int j =0; j< 8; j++){
                                ptr_c[j] = bytes[j];
                            }
                        }break;
                        case PTR_: {
                            uintptr_t *ptr_c = ptr.as.ptr_;
                            u8 * bytes = to_u8(value.as.ptr_);
                            for(int j =0; j< 8; j++){
                                ptr_c[j] = bytes[j];
                            }
                        }break;
                        default: break;
                    }
                }break;
                default: break;
            }
        }break;
        case OP_SHL: {
            StackSlot to_shift, value;
            POP(runtime_stack, value);
            POP(runtime_stack, to_shift);

            if(value.type != INTEGER_){
                PANIC_SHOW_STACK(&op.loc, "Expected integer a the shift amount value");
            }

            if(to_shift.type == PTR_ || to_shift.type == DOUBLE_){
                PANIC_SHOW_STACK(&op.loc, "Cannot use the shift left operator on a pointer or a double. Found: %s",
                                 type_to_str(to_shift.type));
            }

            switch(to_shift.type){
                case CHAR_: {
                    push(&vm->runtime_stack, stackSlot(as_char_(to_shift.as.char_ << value.as.int_), CHAR_));
                }break;
                case IN_PTR_:{
                    push(&vm->runtime_stack, stackSlot(as_short_(to_shift.as.short_ << value.as.int_), IN_PTR_));
                }break;
                case INTEGER_:{
                    push(&vm->runtime_stack, stackSlot(as_int_(to_shift.as.int_ << value.as.int_), INTEGER_));
                }break;
                default: break;
            }

        }break;
        case OP_SHR: {
            StackSlot to_shift, value;
            POP(runtime_stack, value);
            POP(runtime_stack, to_shift);

            if (value.type != INTEGER_) {
                PANIC_SHOW_STACK(&op.loc, "Expected integer a the shift amount value");
            }

            if (to_shift.type == PTR_ || to_shift.type == DOUBLE_) {
                PANIC_SHOW_STACK(&op.loc, "Cannot use the shift right operator on a pointer or a double. Found: %s",
                                 type_to_str(to_shift.type));
            }

            switch (to_shift.type) {
                case CHAR_: {
                    push(&vm->runtime_stack, stackSlot(as_char_(to_shift.as.char_ >> value.as.int_), CHAR_));
                }break;
                case IN_PTR_: {
                    push(&vm->runtime_stack, stackSlot(as_short_(to_shift.as.short_ >> value.as.int_), IN_PTR_));
                }break;
                case INTEGER_: {
                    push(&vm->runtime_stack, stackSlot(as_int_(to_shift.as.int_ >> value.as.int_), INTEGER_));
                }break;
                default: break;
            }
        }break;
        case OP_AND:{
            LOGICAL_OP(&);
        }break;
        case OP_OR:{
            LOGICAL_OP(|);
        }break;
        case OP_NOT:{
            StackSlot to_neg;
            POP(runtime_stack, to_neg);

            if (to_neg.type == PTR_) {
                PANIC_SHOW_STACK(&op.loc, "Cannot negate a pointer");
            }

            switch (to_neg.type) {
                case CHAR_: {
                    push(&vm->runtime_stack, stackSlot(as_char_(!to_neg.as.char_), CHAR_));
                }break;
                case IN_PTR_: {
                    push(&vm->runtime_stack, stackSlot(as_short_(to_neg.as.short_), IN_PTR_));
                }break;
                case INTEGER_: {
                    push(&vm->runtime_stack, stackSlot(as_int_(!to_neg.as.int_), INTEGER_));
                }break;
                case DOUBLE_: {
                    push(&vm->runtime_stack, stackSlot(as_double_(!to_neg.as.double_), DOUBLE_));
                }break;
                default: break;
            }
        }break;
        case OP_NOP:
            break;
        case OP_STACKPRINT:
            print_stack(&vm->runtime_stack);
            break;
        case OP_INTERNAL: {
            StackSlot n_args, id;

            POP(runtime_stack, n_args);

            POP(runtime_stack, id);

            if(n_args.type != INTEGER_){
                PANIC_SHOW_STACK(&op.loc, "Expected integer for the number of arguments of internal op");
            }

            if(id.type != INTEGER_){
                PANIC_SHOW_STACK(&op.loc, "Expected integer for the id of internal op");
            }

            internal_call(vm, n_args.as.int_, id.as.int_);
        }break;
        case OP_END:
            break;
        default: break;
    }
}
size_t ident_n = 0;
size_t label_n = 0;
size_t string_n = 0;
size_t const_n = 0;
void print_op(VM * vm, Op op, bool print_file_info){
    if(print_file_info){
        printf("%s:%zu:%zu: ", CSTR(op.loc.filepath), op.loc.line, op.loc.column);
    }

    if(op.opcode == OP_NOP){
        for(size_t i = 0; i < ident_n; i++){
            printf("  ");
        }
        if(vm->has_dbg_info){
            for(size_t i = 0; i < op.as.bda.count; i++){
                printf("%c", op.as.bda.items[i]);
            }
            printf(": ;; from %s:%zu:%zu", CSTR(op.loc.filepath), op.loc.line, op.loc.column);
        }else {
            printf("label_%zu: ", label_n);
        }
        label_n++;
        ident_n++;
    }else if(op.opcode == OP_END || op.opcode == OP_RET){
        ident_n = 0;
        printf("%s\n", opcode_to_code(op.opcode));
    }else if(op.opcode == OP_STRING_DEF){
        for(size_t i = 0; i < ident_n; i++){
            printf("  ");
        }
        printf("STRING S_%zu ", string_n);
        for(size_t i = 0; i < op.as.bda.count; i++){
            printf("0x%02x", op.as.bda.items[i]);
            if(i < op.as.bda.count - 1){
                printf(", ");
            }
        }
        string_n++;
    }else if(op.opcode == OP_CONST_CHAR_DEF || op.opcode == OP_CONST_INTEGER_DEF || op.opcode == OP_CONST_DOUBLE_DEF){
        for(size_t i = 0; i < ident_n; i++){
            printf("  ");
        }
        printf("CONST C_%zu ", const_n);
        switch(op.type){
            case CHAR_:
                switch(op.as.one){
                    case 0: printf("DC \'\\0\' "); break;
                    case 7: printf("DC \'\\a\' "); break;
                    case 8: printf("DC \'\\b\' "); break;
                    case 9: printf("DC \'\\t\' "); break;
                    case 10: printf("DC \'\\n\' "); break;
                    case 13: printf("DC \'\\r\' "); break;
                    default: printf("DC \'%c\' ", op.as.one);
                }
                break;
            case INTEGER_:
                printf("DI %d ", op.as.four);
                break;
            case DOUBLE_:
                printf("DD %0.13g ", op.as.eight);
                break;
        }
        const_n++;
    }else {
        for(size_t i = 0; i < ident_n; i++){
            printf("  ");
        }
        printf("%s ", opcode_to_code(op.opcode));
        switch(op.type){
            case NONE_:
                break;
            case BYTE_:
                printf("0x%02x ", op.as.one);
                break;
            case CHAR_:
                switch(op.as.one){
                    case 0: printf("\'\\0\' "); break;
                    case 7: printf("\'\\a\' "); break;
                    case 8: printf("\'\\b\' "); break;
                    case 9: printf("\'\\t\' "); break;
                    case 10: printf("\'\\n\' "); break;
                    case 13: printf("\'\\r\' "); break;
                    default: printf("\'%c\' ", op.as.one);
                }
                break;
            case SHORT_:
                printf("0x%04x ", op.as.two);
                break;
            case IN_PTR_:
                printf("0x%02x ", op.as.two);
                break;
            case INTEGER_:
                printf("%d ", op.as.four);
                break;
            case PTR_:
                break;
            case DOUBLE_:
                printf("%0.13g ", op.as.eight);
                break;
            case BYTE_ARRAY_:
                for(size_t i = 0; i < op.as.bda.count; i++){
                    printf("0x%02x", op.as.bda.items[i]);
                    if(i < op.as.bda.count - 1){
                        printf(", ");
                    }
                }
                printf(";; \"");
                for(size_t i = 0; i < op.as.bda.count; i++){
                    switch(op.as.bda.items[i]){
                        case 0: printf("\\0"); break;
                        case 7: printf("\\a"); break;
                        case 8: printf("\\b"); break;
                        case 9: printf("\\t"); break;
                        case 10: printf("\\n"); break;
                        case 13: printf("\\r"); break;
                        default: printf("%c", op.as.bda.items[i]);
                    }
                }
                printf("\"");
                break;
        }
    }
    if(vm->has_dbg_info && ((op.opcode >= 27 && op.opcode <= 35) || op.opcode == 40)){
        printf(";; jump to -> ");
        for(size_t i = 0; i < vm->codeDA.items[op.as.two].as.bda.count; i++){
            printf("%c", vm->codeDA.items[op.as.two].as.bda.items[i]);
        }
    }

    if(vm->has_dbg_info && op.is_extended){
        printf(";;");
        for(size_t i = 0; i < op.exd_da_ext.count; i++){
            printf(" extended from ");
            printf("%s:%zu:%zu ",
                   CSTR(op.exd_da_ext.items[i].filepath),
                   op.exd_da_ext.items[i].line,
                   op.exd_da_ext.items[i].column);
            if(i < op.exd_da_ext.count - 1){
                printf("->");
            }
        }
    }
    printf("\n");
}

void vm_disassemble(VM * vm, bool print_file_info){
    if(vm->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }
    size_t line = 0;

    printf("ENTRY 0x%04x\n\n", vm->entry_point);

    for(size_t i = 0; i < vm->data_chunk_size; i++ ,line++){
        Op op = vm->dataDA.items[i];
        print_op(vm, op, print_file_info);
    }

    for(size_t i = 0; i < vm->code_chunk_size; i++ ,line++){
        Op op = vm->codeDA.items[i];
        print_op(vm, op, print_file_info);
    }

    if(vm->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        log_(INFO, NULL, "Disassemble took %lf seconds.", (timespecDiff(&end, &start) * 0.000000001));
    }

}