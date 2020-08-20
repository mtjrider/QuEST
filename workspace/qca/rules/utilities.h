#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "../../../QuEST/include/QuEST.h"

#pragma once
#if !defined UTILITIES_H
#define UTILITIES_H
#define INVALID UINT_MAX

void printStatusMessage(char * message) {
        char buffer[80];
        int length = sprintf(buffer, "| %s |", message);
        char pad = '-';
        char padding[length+1];
        for (int i = 0; i < length; ++i) {
            padding[i] = pad;
        }
        padding[length] = '\0';
        printf("%s\n", padding);
        printf("%s\n", buffer);
        printf("%s\n", padding);
}

unsigned int * appendArray(unsigned int * arr,
                 unsigned int * vals,
                 unsigned int nvals) {
    unsigned int len = 0;
    if (arr != NULL) {
        len = arr[0];
    }
    arr = (unsigned int *) realloc(arr, (len+nvals+1)*sizeof(unsigned int));
    for (unsigned int i = 0; i < nvals; ++i) {
        arr[len+i+1] = vals[i];
    }
    arr[0] = len + nvals;
    return arr;
}

unsigned int * initArray() {
    unsigned int nout = 0, nvals = 0;
    unsigned int * out = NULL;
    out = appendArray(out, &nout, nvals);
    return out;
}

unsigned int getStateIndex(char * bitstring,
                           unsigned int base) {
    return strtoull(bitstring, NULL, base);
}

char * initBitString(unsigned int len) {
    char * out = (char *) calloc(len, sizeof(char));
    for (unsigned int i = 0; i < len; ++i) {
        out[i] = '0';
    }
    return out;
}

unsigned int nCr(unsigned int n,
                 unsigned int r) {
    if (n < r) {
        return 0;
    }
    if (n < 2 * r) {
        r = n - r;
    }
    if (r == 0) {
        return 1;
    }
    unsigned int result = n;
    for(unsigned int i = 2; i <= r; ++i) {
        result *= (n - i + 1);
        result /= i;
    }
    return result;
}

char * getBinary(unsigned int val,
                 unsigned int nbits) {
    char * out = (char *) calloc(nbits, sizeof(char));
    for (unsigned int i = 0; i < nbits; ++i) {
        out[i] = '0';
    }
    unsigned int index = 0;
    for (unsigned int i = 1<<(nbits-1); i > 0; i = i/2) {
        out[index] = (val & i) ? '1' : '0';
        index = index + 1;
    }
    return out;
}

unsigned int getTarget(unsigned int * neighborhood) {
    unsigned int center = (unsigned int) floor(neighborhood[0] / 2);
    unsigned int out = neighborhood[center+1];
    return out;
}

unsigned int * getControls(unsigned int * neighborhood) {
    unsigned int * out = initArray();
    unsigned int val, nvals = 1;
    unsigned int center = (unsigned int) floor(neighborhood[0] / 2);
    for (unsigned int i = 0; i < neighborhood[0]; ++i) {
        if (i != center) {
            val = neighborhood[i+1];
            if (val != INVALID) {
                out = appendArray(out, &val, nvals);
            }
        }
    }
    return out;
}

unsigned int getLevel(char * neighbors,
                      unsigned int nhood) {
    unsigned int out = 0, base = 10;
    for (unsigned int i = 0; i < nhood-1; ++i) {
        char level = neighbors[i];
        out = out + strtoull(&level, NULL, base);
    }
    return out;
}

unsigned int * getLevels(unsigned int rule,
                         unsigned int nhood) {
    unsigned int val, nvals = 1;
    unsigned int * out = initArray();
    char * binrule = getBinary(rule, nhood);
    for (unsigned int i = 0; i < nhood; ++i) {
        char bit = binrule[i];
        val = (nhood - 1) - i;
        if (bit == '1') {
            out = appendArray(out, &val, nvals);
        }
    }
    free(binrule);
    return out;
}


char ** getTotalisticRule(unsigned int level,
                          unsigned int nhood) {
    unsigned int nvalid = nCr(nhood-1, level);
    unsigned int nnhoods = (unsigned int) pow(2, nhood-1);
    char * neighbors = (char *) calloc(nhood-1, sizeof(char));
    char ** out = (char **) calloc(nvalid, sizeof(char *));
    for (unsigned int i = 0; i < nvalid; ++i) {
        out[i] = (char *) calloc(nhood, sizeof(char));
        for (unsigned int j = 0; j < nhood; ++j) {
            out[i][j] = '0';
        }
    }
    unsigned int index = 0;
    unsigned int center = (unsigned int) floor(nhood / 2);
    for (unsigned int i = 0; i < nnhoods; ++i) {
        neighbors = getBinary(i, nhood-1);
        if (getLevel(neighbors, nhood) == level) {
            for (unsigned int j = 0; j < nhood-1; ++j) {
                if (j < center) {
                    out[index][j] = neighbors[j];
                } else {
                    out[index][j+center-1] = neighbors[j];
                }
            }
            index = index + 1;
        }
    }
    free(neighbors);
    return out;
}

unsigned int * getPauliXTargets(char * signature,
                                unsigned int * controls) {
    unsigned int * out = initArray();
    unsigned int val, nvals = 1;
    unsigned int center = (unsigned int) floor(controls[0]+1 / 2);
    for (unsigned int i = 0; i < controls[0]+1; ++i) {
        char bit = '\0';
        if (i != center) {
            bit = signature[i];
        }
        if (bit == '0') {
            if (i < center) {
                val = controls[i+1];
            }
            if (center < i) {
                val = controls[i+center];
            }
            out = appendArray(out, &val, nvals);
        }
    }
    return out;
}

void multiPauliX(Qureg qubits,
                 unsigned int * targets) {
    for (unsigned int i = 0; i < targets[0]; ++i) {
        pauliX(qubits, targets[i+1]);
    }
}

ComplexMatrix2 getActivator(char * name) {
    ComplexMatrix2 activator;
    if (!strcmp(name, "hadamard")) {
        double scale = 1/sqrt(2);
        activator = (ComplexMatrix2) {
            .real={{scale,  scale},
                   {scale, -scale}},
            .imag={{0,  0},
                   {0,  0}}
        };
    }
    return activator;
}

void multiControlledActivator(Qureg qubits,
                              unsigned int * controls,
                              unsigned int target,
                              unsigned int level,
                              ComplexMatrix2 activator) {
    unsigned int nsignatures = nCr(controls[0], level);
    char ** signatures = getTotalisticRule(level, controls[0]+1);
    for (unsigned int i = 0; i < nsignatures; ++i) {
        char * signature = signatures[i];
        unsigned int * ptargets = getPauliXTargets(signature, controls);
        multiPauliX(qubits, ptargets);
        multiControlledUnitary(qubits, (int *) &controls[1], controls[0], target, activator);
        multiPauliX(qubits, ptargets);
        free(ptargets);
    }
    free(signatures);
}

#endif
