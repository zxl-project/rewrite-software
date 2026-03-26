/* * Refactored JSON Utility Methods 
 * Functionally equivalent to cJSON_Utils but structurally rewritten.
 */

 #if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
 #define _CRT_SECURE_NO_DEPRECATE
 #endif
 
 #ifdef __GNUCC__
 #pragma GCC visibility push(default)
 #endif
 
 #include <ctype.h>
 #include <string.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <limits.h>
 #include <math.h>
 #include <float.h>
 
 #ifdef __GNUCC__
 #pragma GCC visibility pop
 #endif
 
 #include "cJSON_Utils.h"
 
 /* Boolean Definitions Re-mapped */
 #ifndef CJSON_TRUE_VAL
 #define CJSON_TRUE_VAL ((cJSON_bool)1)
 #define CJSON_FALSE_VAL ((cJSON_bool)0)
 #endif
 
 /* ====================================================================
  * Memory & String Helpers
  * ==================================================================== */
 
 static unsigned char* CloneStrEx(const unsigned char* const srcText)
 {
     unsigned char *cloned = NULL;
     size_t memSize = 0;
 
     if (!srcText) return NULL;
 
     memSize = strlen((const char*)srcText) + 1; /* Include null terminator */
     cloned = (unsigned char*) cJSON_malloc(memSize);
     
     if (cloned) {
         memcpy(cloned, srcText, memSize);
     }
     return cloned;
 }
 
 static int CheckStrEquality(const unsigned char *s1, const unsigned char *s2, const cJSON_bool isCaseSensitive)
 {
     if (!s1 || !s2) return 1; /* Treat NULLs as unequal */
     if (s1 == s2) return 0;   /* Same memory address */
 
     if (isCaseSensitive) {
         return strcmp((const char*)s1, (const char*)s2);
     }
 
     /* Case-insensitive evaluation loop */
     while (tolower(*s1) == tolower(*s2)) {
         if (*s1 == '\0') {
             return 0;
         }
         s1++;
         s2++;
     }
 
     return (tolower(*s1) - tolower(*s2));
 }
 
 static cJSON_bool AreDoublesEqual(double valA, double valB)
 {
     double absA = fabs(valA);
     double absB = fabs(valB);
     double maxMagnitude = (absA > absB) ? absA : absB;
     
     return (fabs(valA - valB) <= maxMagnitude * DBL_EPSILON) ? CJSON_TRUE_VAL : CJSON_FALSE_VAL;
 }
 
 /* ====================================================================
  * JSON Pointer Core Logic
  * ==================================================================== */
 
 static cJSON_bool VerifyPointerMatch(const unsigned char *nodeName, const unsigned char *ptrPath, const cJSON_bool caseSens)
 {
     if (!nodeName || !ptrPath) return CJSON_FALSE_VAL;
 
     while ((*nodeName != '\0') && (*ptrPath != '\0') && (*ptrPath != '/')) 
     {
         if (*ptrPath == '~') {
             /* Handle JSON Pointer escaping */
             cJSON_bool isZeroEscape = (ptrPath[1] == '0' && *nodeName == '~');
             cJSON_bool isOneEscape = (ptrPath[1] == '1' && *nodeName == '/');
             
             if (!isZeroEscape && !isOneEscape) {
                 return CJSON_FALSE_VAL;
             }
             ptrPath += 2; /* Skip the escape sequence */
             nodeName++;
             continue;
         } 
         
         cJSON_bool charMismatch = caseSens ? (*nodeName != *ptrPath) : (tolower(*nodeName) != tolower(*ptrPath));
         if (charMismatch) {
             return CJSON_FALSE_VAL;
         }
         
         nodeName++;
         ptrPath++;
     }
 
     /* Ensure both strings terminated at the boundary */
     cJSON_bool ptrEnded = (*ptrPath == 0 || *ptrPath == '/');
     cJSON_bool nameEnded = (*nodeName == 0);
     
     return (ptrEnded == nameEnded) ? CJSON_TRUE_VAL : CJSON_FALSE_VAL;
 }
 
 static size_t CalculateEscapedLen(const unsigned char *rawStr)
 {
     size_t totalLen = 0;
     while (*rawStr) {
         totalLen += (*rawStr == '~' || *rawStr == '/') ? 2 : 1;
         rawStr++;
     }
     return totalLen;
 }
 
 static void FormatAsJsonPointer(unsigned char *destBuf, const unsigned char *srcStr)
 {
     while (*srcStr) {
         if (*srcStr == '/') {
             *destBuf++ = '~';
             *destBuf++ = '1';
         } else if (*srcStr == '~') {
             *destBuf++ = '~';
             *destBuf++ = '0';
         } else {
             *destBuf++ = *srcStr;
         }
         srcStr++;
     }
     *destBuf = '\0';
 }
 
 CJSON_PUBLIC(char *) cJSONUtils_FindPointerFromObjectTo(const cJSON * const rootNode, const cJSON * const targetNode)
 {
     cJSON *currNode = NULL;
     size_t idx = 0;
 
     if (!rootNode || !targetNode) return NULL;
     if (rootNode == targetNode) return (char*)CloneStrEx((const unsigned char*)"");
 
     currNode = rootNode->child;
     while (currNode) 
     {
         unsigned char *subPath = (unsigned char*)cJSONUtils_FindPointerFromObjectTo(currNode, targetNode);
         
         if (subPath) 
         {
             unsigned char *fullPath = NULL;
             
             if (cJSON_IsArray(rootNode)) {
                 if (idx > ULONG_MAX) {
                     cJSON_free(subPath);
                     return NULL;
                 }
                 fullPath = (unsigned char*)cJSON_malloc(strlen((char*)subPath) + 24); /* Sufficient buffer */
                 sprintf((char*)fullPath, "/%lu%s", (unsigned long)idx, subPath);
             } 
             else if (cJSON_IsObject(rootNode)) {
                 size_t reqLen = strlen((char*)subPath) + CalculateEscapedLen((unsigned char*)currNode->string) + 2;
                 fullPath = (unsigned char*)cJSON_malloc(reqLen);
                 fullPath[0] = '/';
                 FormatAsJsonPointer(fullPath + 1, (unsigned char*)currNode->string);
                 strcat((char*)fullPath, (char*)subPath);
             }
 
             cJSON_free(subPath);
             return (char*)fullPath;
         }
         currNode = currNode->next;
         idx++;
     }
     return NULL;
 }
 
 /* ====================================================================
  * Array & Pointer Retrieval Refactored
  * ==================================================================== */
 
 static cJSON *FetchArrayElement(const cJSON *arrNode, size_t targetIdx)
 {
     if (!arrNode) return NULL;
     
     cJSON *cursor = arrNode->child;
     for (size_t i = 0; i < targetIdx && cursor != NULL; i++) {
         cursor = cursor->next;
     }
     return cursor;
 }
 
 static cJSON_bool ParseIdxFromPath(const unsigned char * const pathStr, size_t * const outIdx)
 {
     size_t calcIdx = 0;
     int i = 0;
 
     /* Reject leading zeros */
     if (pathStr[0] == '0' && pathStr[1] != '\0' && pathStr[1] != '/') return CJSON_FALSE_VAL;
 
     while (pathStr[i] >= '0' && pathStr[i] <= '9') {
         calcIdx = (calcIdx * 10) + (pathStr[i] - '0');
         i++;
     }
 
     /* If trailing chars exist before next token, it's invalid */
     if (pathStr[i] != '\0' && pathStr[i] != '/') return CJSON_FALSE_VAL;
 
     *outIdx = calcIdx;
     return CJSON_TRUE_VAL;
 }
 
 static cJSON *ResolvePointerPath(cJSON * const baseObj, const char * path, const cJSON_bool strictCase)
 {
     cJSON *navNode = baseObj;
 
     if (!path) return NULL;
 
     while (*path == '/' && navNode) 
     {
         path++; /* Move past the slash */
         
         if (cJSON_IsArray(navNode)) {
             size_t arrIdx = 0;
             if (!ParseIdxFromPath((const unsigned char*)path, &arrIdx)) return NULL;
             navNode = FetchArrayElement(navNode, arrIdx);
         } 
         else if (cJSON_IsObject(navNode)) {
             navNode = navNode->child;
             while (navNode) {
                 if (VerifyPointerMatch((unsigned char*)navNode->string, (const unsigned char*)path, strictCase)) {
                     break;
                 }
                 navNode = navNode->next;
             }
         } 
         else {
             return NULL; /* Cannot traverse primitives */
         }
 
         /* Fast-forward to next segment */
         while (*path != '\0' && *path != '/') path++;
     }
 
     return navNode;
 }
 
 CJSON_PUBLIC(cJSON *) cJSONUtils_GetPointer(cJSON * const object, const char *pointer) {
     return ResolvePointerPath(object, pointer, CJSON_FALSE_VAL);
 }
 
 CJSON_PUBLIC(cJSON *) cJSONUtils_GetPointerCaseSensitive(cJSON * const object, const char *pointer) {
     return ResolvePointerPath(object, pointer, CJSON_TRUE_VAL);
 }