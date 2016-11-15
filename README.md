# Fend - Sandbox

Restricts the child program's access based on the entries in the config file.

Config file working
-------------------
If no config provided as params, checks the current directory for .fendrc. If not found checks in ~/.fendrc. If not found, throws an error

Config file structure
---------------------
{mode} {file}

mode is a octal 3 digit number (RWX). 
Eg. 
777 represents all access to the {file}
000 represents no access to the {file}



Running
-------
./fend {absolute path to the program}
or
./fend {absolute path to the program} [-c {config file name}]
