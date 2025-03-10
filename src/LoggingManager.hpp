#pragma once

#define enquote(S) ('"' + (S) + '"')
#define enquoteCOUT(S) '"' << (S) << '"'
#define VARIABLE_NAME(V) (#V)