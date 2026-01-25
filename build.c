#include "redbuild.h"

#include "syscalls/syscalls.h"

void gpu(){
	new_module("gpu");
	set_name("rend");
	set_package_type(package_red);
	set_target(target_native);
	source("obj.c");
	source("main.c");
	if (compile()){
		run();
	}
	
}

void main(){
	gpu();
}
