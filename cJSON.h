/*
 * Extended JSON Library for C (Base Module)
 * A restructured implementation of a lightweight JSON parser.
 */

 #ifndef JSON_CORE_ENGINE_H
 #define JSON_CORE_ENGINE_H
 
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* --- 平台兼容性与导出宏重构 --- */
 #if !defined(_WIN32) && (defined(WIN32) || defined(_MSC_VER) || defined(__WIN32__))
     #define _NODE_PLATFORM_WINDOWS
 #endif
 
 #if defined(_NODE_PLATFORM_WINDOWS)
     #define NODE_CDECL __cdecl
     #if defined(NODE_DLL_EXPORT)
         #define NODE_PUBLIC(type) __declspec(dllexport) type __stdcall
     #elif defined(NODE_DLL_IMPORT)
         #define NODE_PUBLIC(type) __declspec(dllimport) type __stdcall
     #else
         #define NODE_PUBLIC(type) type __stdcall
     #endif
 #else
     #define NODE_CDECL
     #if defined(__GNUC__) && defined(NODE_VISIBILITY_ENABLE)
         #define NODE_PUBLIC(type) __attribute__((visibility("default"))) type
     #else
         #define NODE_PUBLIC(type) type
     #endif
 #endif
 
 /* --- 核心版本定义 --- */
 #define ENGINE_VER_MAJOR 1
 #define ENGINE_VER_MINOR 7
 #define ENGINE_VER_REV   19
 
 /* --- 节点类型定义 (重新映射数值) --- */
 #define J_TYPE_INVALID   0
 #define J_TYPE_FALSE     (1 << 0)
 #define J_TYPE_TRUE      (1 << 1)
 #define J_TYPE_NULL      (1 << 2)
 #define J_TYPE_NUMBER    (1 << 3)
 #define J_TYPE_STRING    (1 << 4)
 #define J_TYPE_ARRAY     (1 << 5)
 #define J_TYPE_OBJECT    (1 << 6)
 #define J_TYPE_RAW_DATA  (1 << 7)
 
 #define J_FLAG_REF       (1 << 8)
 #define J_FLAG_CONST_KEY (1 << 9)
 
 /* --- 核心数据结构 --- */
 
 /* 重新排序字段以改变结构体特征 */
 typedef struct JsonNode {
     int flags;                 /* 原 type */
     char *key;                 /* 原 string (键名) */
     
     struct JsonNode *next;     /* 链表指针 */
     struct JsonNode *prev;
     struct JsonNode *child;    /* 容器子项 */
 
     char *val_str;             /* 字符串值 */
     double val_double;         /* 浮点值 */
     int val_int;               /* 整数值 */
 } JsonNode;
 
 typedef struct {
     void *(NODE_CDECL *alloc_func)(size_t size);
     void (NODE_CDECL *release_func)(void *ptr);
 } JsonHooks;
 
 typedef int j_bool;
 
 /* 递归深度限制 */
 #ifndef J_MAX_NESTING
 #define J_MAX_NESTING 1000
 #endif
 
 /* --- API 函数集 --- */
 
 /* 库信息 */
 NODE_PUBLIC(const char*) Json_GetVersion(void);
 NODE_PUBLIC(void)        Json_SetupMemory(JsonHooks* hooks);
 
 /* 解析接口 */
 NODE_PUBLIC(JsonNode *)  Json_ParseText(const char *input);
 NODE_PUBLIC(JsonNode *)  Json_ParseTextLen(const char *input, size_t len);
 NODE_PUBLIC(JsonNode *)  Json_ParseExt(const char *input, const char **err_ptr, j_bool final_null);
 
 /* 序列化接口 */
 NODE_PUBLIC(char *)      Json_Stringify(const JsonNode *node);
 NODE_PUBLIC(char *)      Json_StringifyRaw(const JsonNode *node);
 NODE_PUBLIC(char *)      Json_StringifyBuffered(const JsonNode *node, int prealloc_size, j_bool format);
 NODE_PUBLIC(j_bool)      Json_StringifyStatic(JsonNode *node, char *buffer, const int buf_len, const j_bool format);
 
 /* 释放内存 */
 NODE_PUBLIC(void)        Json_DeleteNode(JsonNode *node);
 
 /* 查询与检索 */
 NODE_PUBLIC(int)         Json_ArrayCount(const JsonNode *array);
 NODE_PUBLIC(JsonNode *)  Json_GetArrayIdx(const JsonNode *array, int idx);
 NODE_PUBLIC(JsonNode *)  Json_GetObjField(const JsonNode * const obj, const char * const name);
 NODE_PUBLIC(JsonNode *)  Json_GetObjFieldStrict(const JsonNode * const obj, const char * const name);
 NODE_PUBLIC(j_bool)      Json_HasField(const JsonNode *obj, const char *name);
 NODE_PUBLIC(const char *) Json_GetErrorOffset(void);
 
 /* 数据读取 */
 NODE_PUBLIC(char *)      Json_ReadString(const JsonNode * const node);
 NODE_PUBLIC(double)      Json_ReadNumber(const JsonNode * const node);
 
 /* 类型检查宏重写 */
 #define Json_IsNodeValid(n)  ((n) && (((n)->flags & 0xFF) != J_TYPE_INVALID))
 #define Json_IsNodeNull(n)   ((n) && (((n)->flags & 0xFF) == J_TYPE_NULL))
 #define Json_IsNodeString(n) ((n) && (((n)->flags & 0xFF) == J_TYPE_STRING))
 #define Json_IsNodeArray(n)  ((n) && (((n)->flags & 0xFF) == J_TYPE_ARRAY))
 #define Json_IsNodeObject(n) ((n) && (((n)->flags & 0xFF) == J_TYPE_OBJECT))
 
 /* 节点创建 */
 NODE_PUBLIC(JsonNode *)  Json_NewNull(void);
 NODE_PUBLIC(JsonNode *)  Json_NewTrue(void);
 NODE_PUBLIC(JsonNode *)  Json_NewFalse(void);
 NODE_PUBLIC(JsonNode *)  Json_NewBool(j_bool b);
 NODE_PUBLIC(JsonNode *)  Json_NewNumber(double num);
 NODE_PUBLIC(JsonNode *)  Json_NewString(const char *str);
 NODE_PUBLIC(JsonNode *)  Json_NewArray(void);
 NODE_PUBLIC(JsonNode *)  Json_NewObject(void);
 
 /* 节点修改与维护 */
 NODE_PUBLIC(j_bool)      Json_LinkToArray(JsonNode *array, JsonNode *node);
 NODE_PUBLIC(j_bool)      Json_LinkToObject(JsonNode *obj, const char *key, JsonNode *node);
 NODE_PUBLIC(JsonNode *)  Json_UnlinkByIndex(JsonNode *array, int idx);
 NODE_PUBLIC(JsonNode *)  Json_UnlinkByKey(JsonNode *obj, const char *key);
 
 /* 辅助宏 */
 #define Json_ForEach(item, array) for(item = (array != NULL) ? (array)->child : NULL; item != NULL; item = item->next)
 
 /* 手动内存管理 */
 NODE_PUBLIC(void *)      Json_AllocRaw(size_t sz);
 NODE_PUBLIC(void)        Json_FreeRaw(void *p);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* JSON_CORE_ENGINE_H */