#!/usr/bin/env python2

import fileinput, re

modules = []
partten = re.compile(r"^\bDECLEAR_SERVICE\(([0-9A-Za-z_]*)\)")
for line in fileinput.input():
    o = partten.match(line)
    if o: modules.extend(o.groups())

print "struct evops;"
for m in modules: print "extern struct evops %s;" % m
print "struct evops* services[] = {"
for m in modules: print "\t&%s," % m
print "\t(void*)0,"
print "};"
