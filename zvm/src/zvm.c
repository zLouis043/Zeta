#include "common/Log.h"
#include "common/Configs.h"
#include "common/ConfigFlags.h"
#include "VM.h"

int main(int argc, char **argv){

    if(argc < 2){
        PANIC(GENERIC_ERROR, NULL, "[USAGE] ./zvm <input_file.zasm> [SUBCOMMANDS]\n[USAGE] Use the -help flag to see all the subcommands.");
    }

    flag_shift_args(&argc, &argv);

    SV filepath = SV(flag_shift_args(&argc, &argv));

    bool * time = flag_bool("time", "Get each operation time", false);
    bool * help = flag_bool("help", "Get all the subcommands", false);
    bool * dis =  flag_bool("dis", "Print the disassembled program", false);
    bool * dgb = flag_bool("dinfo", "Include debug information during the disassemble printing", false);
    if(!flag_parse(argc, argv)){
        flag_print_flags(stdout);
    }

    if(*help){
        flag_print_flags(stdout);
    }

    Configs conf = {
            .filepath = filepath,
            .dump_op_codes = false,
            .time_exec = *time,
            .is_dgb = false
    };

    VM vm = {0};
    vm_init(&vm, CSTR(filepath), conf);
    vm_gather(&vm);
    int exit_value = 0;

    if(*dis){
        vm_disassemble(&vm, *dgb);
    }else{
        exit_value = vm_run(&vm);
    }
    return exit_value;
}