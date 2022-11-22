#ifndef COMMON_H
#define COMMON_H

const int ERROR = -1;
const int MAX_NODE_LEN = 12;

#define _UNWRAP_NULL(cond)     { if ((cond) == NULL)  { return NULL;  } }
#define _UNWRAP_NULL_ERR(cond) { if ((cond) == NULL)  { return ERROR; } }
#define _UNWRAP_ERR(expr)      { if ((expr) == ERROR) { return ERROR; } }
#define _UNWRAP_ERR_NULL(cond) { if ((cond) == ERROR) { return NULL;  } }

#endif
