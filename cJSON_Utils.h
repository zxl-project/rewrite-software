/*
 * Refactored Extended Utilities for cJSON
 * Provides JSON Pointer, Patch, and Merge Patch capabilities.
 * Compatible with standard cJSON structures.
 */

 #ifndef CJSON_UTILITY_EXTENSIONS_H_
 #define CJSON_UTILITY_EXTENSIONS_H_
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #include "cJSON.h"
 
 /* ====================================================================
  * JSON Pointer Operations (Based on RFC 6901)
  * ==================================================================== */
 
 /* Resolves a JSON pointer path against a given root object */
 CJSON_PUBLIC(cJSON *) cJSONExt_ResolvePointer(cJSON * const rootNode, const char *pathStr);
 CJSON_PUBLIC(cJSON *) cJSONExt_ResolvePointerStrict(cJSON * const rootNode, const char *pathStr);
 
 /* Computes the pointer string required to navigate from source to target node */
 CJSON_PUBLIC(char *)  cJSONExt_GetPathFromNode(const cJSON * const sourceObj, const cJSON * const targetObj);
 
 
 /* ====================================================================
  * JSON Patch Operations (Based on RFC 6902)
  *
  * NOTE: The generation functions will internally sort the children of 
  * both input objects. Pass duplicates if immutability is required.
  * ==================================================================== */
 
 /* Generates a patch sequence mapping 'source' to 'destination' */
 CJSON_PUBLIC(cJSON *) cJSONExt_CreateDiffPatch(cJSON * const source, cJSON * const destination);
 CJSON_PUBLIC(cJSON *) cJSONExt_CreateDiffPatchStrict(cJSON * const source, cJSON * const destination);
 
 /* Helper to manually construct patch arrays */
 CJSON_PUBLIC(void)    cJSONExt_AppendPatchOperation(cJSON * const patchArray, const char * const opType, const char * const targetPath, const cJSON * const valNode);
 
 /* Executes a patch array on a target object. Returns 0 upon successful complete execution. */
 CJSON_PUBLIC(int)     cJSONExt_ExecutePatch(cJSON * const targetObj, const cJSON * const patchSequence);
 CJSON_PUBLIC(int)     cJSONExt_ExecutePatchStrict(cJSON * const targetObj, const cJSON * const patchSequence);
 
 /*
  * [Design Note on Atomicity]
  * The ExecutePatch functions are NOT atomic. A failure halfway through will leave 
  * the targetObj partially modified. If transactional safety is required in your app, 
  * you must duplicate the object (cJSON_Duplicate), apply the patch to the clone, 
  * and swap pointers only if the operation returns 0.
  */
 
 
 /* ====================================================================
  * JSON Merge Patch Operations (Based on RFC 7386)
  * ==================================================================== */
 
 /* Applies a merge patch. Target is mutated directly. Returns the updated target ptr. */
 CJSON_PUBLIC(cJSON *) cJSONExt_ApplyMergePatch(cJSON *targetObj, const cJSON * const mergePatch);
 CJSON_PUBLIC(cJSON *) cJSONExt_ApplyMergePatchStrict(cJSON *targetObj, const cJSON * const mergePatch);
 
 /* Generates a merge patch. As with RFC 6902 generation, inputs are sorted in-place. */
 CJSON_PUBLIC(cJSON *) cJSONExt_BuildMergePatch(cJSON * const source, cJSON * const destination);
 CJSON_PUBLIC(cJSON *) cJSONExt_BuildMergePatchStrict(cJSON * const source, cJSON * const destination);
 
 
 /* ====================================================================
  * Internal Structure Utilities
  * ==================================================================== */
 
 /* Alphabetizes the child nodes of an object based on their string keys. */
 CJSON_PUBLIC(void) cJSONExt_AlphabetizeKeys(cJSON * const targetObj);
 CJSON_PUBLIC(void) cJSONExt_AlphabetizeKeysStrict(cJSON * const targetObj);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* CJSON_UTILITY_EXTENSIONS_H_ */