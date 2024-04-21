#include "common/Log.h"
#include "common/Configs.h"
#include "common/ConfigFlags.h"
#include "Generator.h"

#include <sys/time.h>


int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
{
    return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

int main(int argc, char **argv){

    if(argc < 2){
        PANIC(GENERIC_ERROR, NULL, "[USAGE] ./ZASM <input_file.zasm> [SUBCOMMANDS]\n[USAGE] Use the -help flag to see all the subcommands.");
    }

    flag_shift_args(&argc, &argv);

    SV filepath = SV(flag_shift_args(&argc, &argv));

    bool * dump = flag_bool("dump", "Print all the opcodes of the program", false);
    bool * time = flag_bool("time", "Get each operation time", false);
    bool * dgb = flag_bool("dinfo", "Include debug information", false);
    bool * help = flag_bool("help", "Get all the subcommands", false);
    char ** output_file = flag_str("o", "Set the output file.", NULL);

    if(!flag_parse(argc, argv)){
        flag_print_flags(stdout);
    }

    if(*help){
        flag_print_flags(stdout);
    }

    Configs conf = {
            .filepath = filepath,
            .dump_op_codes = *dump,
            .time_exec = *time,
            .is_dgb = *dgb
    };

    if(*output_file){
        conf.output_path = SV(*output_file);
    }else{
        conf.output_path.data = NULL;
        conf.output_path.len = 0;
    }

    Generator gen = {0};

    struct timespec start, end;

    if(conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    generator_init(&gen, conf);

    int exit_value = generate(&gen);

    if(conf.time_exec) {
        if(exit_value == 0) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            log_(INFO, NULL, "Compiling took %lf seconds.", (timespecDiff(&end, &start) * 0.000000001));
        }else{
            log_(GENERIC_ERROR, NULL, "Compilation failed.");
        }
    }

    return exit_value;
}