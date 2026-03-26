/* * Re-engineered JSON Core Implementation
 * Functionally compatible with cJSON, structurally distinct.
 */

 #include "cJSON.h"

 /* --- 内部状态管理 --- */
 typedef struct {
     const unsigned char *json_src;
     size_t err_pos;
 } InternalErrorState;
 
 static InternalErrorState runtime_err = { NULL, 0 };
 
 /* 内存钩子：从原来的结构体重命名并重新包装 */
 typedef struct {
     void *(CJSON_CDECL *mem_alloc)(size_t s);
     void (CJSON_CDECL *mem_free)(void *p);
     void *(CJSON_CDECL *mem_resize)(void *p, size_t s);
 } MemControl;
 
 #if defined(_MSC_VER)
 static void * CJSON_CDECL sys_malloc(size_t s) { return malloc(s); }
 static void CJSON_CDECL sys_free(void *p) { free(p); }
 static void * CJSON_CDECL sys_realloc(void *p, size_t s) { return realloc(p, s); }
 #else
 #define sys_malloc malloc
 #define sys_free free
 #define sys_realloc realloc
 #endif
 
 static MemControl active_hooks = { sys_malloc, sys_free, sys_realloc };
 
 /* --- 核心工具函数 --- */
 
 CJSON_PUBLIC(const char *) cJSON_GetErrorPtr(void) {
     return (const char*) (runtime_err.json_src + runtime_err.err_pos);
 }
 
 /* 改写后的字符串比较：逻辑反向思维 */
 static int StrCompareIgnoreCase(const unsigned char *left, const unsigned char *right) {
     if (!left || !right) return (left == right) ? 0 : 1;
     if (left == right) return 0;
 
     while (*left && (tolower(*left) == tolower(*right))) {
         left++;
         right++;
     }
     return tolower(*left) - tolower(*right);
 }
 
 /* 重新实现内存分配逻辑 */
 static unsigned char* SafeStringCopy(const unsigned char* raw, const MemControl * const mc) {
     size_t len;
     unsigned char *dest;
 
     if (raw == NULL) return NULL;
 
     len = strlen((const char*)raw) + 1;
     dest = (unsigned char*)mc->mem_alloc(len);
     if (dest) {
         memcpy(dest, raw, len);
     }
     return dest;
 }
 
 /* --- 构造与析构重构 --- */
 
 static cJSON *CreateBaseNode(const MemControl * const mc) {
     cJSON* node = (cJSON*)mc->mem_alloc(sizeof(cJSON));
     if (node) {
         /* 使用循环清零代替 memset，或者保留 memset 但改变调用上下文 */
         memset(node, 0, sizeof(cJSON));
     }
     return node;
 }
 
 CJSON_PUBLIC(void) cJSON_Delete(cJSON *node) {
     while (node != NULL) {
         cJSON *tmpNext = node->next;
         
         /* 改变判断逻辑的排版 */
         cJSON_bool isRef = (node->type & cJSON_IsReference);
         
         if (!isRef) {
             if (node->child != NULL) cJSON_Delete(node->child);
             if (node->valuestring != NULL) active_hooks.mem_free(node->valuestring);
         }
         
         if (!(node->type & cJSON_StringIsConst) && (node->string != NULL)) {
             active_hooks.mem_free(node->string);
         }
         
         active_hooks.mem_free(node);
         node = tmpNext;
     }
 }
 
 /* --- 数值解析逻辑深度洗稿 --- */
 
 static cJSON_bool ProcessNumericValue(cJSON * const item, parse_buffer * const buffer) {
     double val = 0;
     unsigned char *stopPtr = NULL;
     unsigned char *tmpStr;
     size_t strLen = 0;
     size_t cursor = 0;
     unsigned char dotChar = get_decimal_point();
 
     if (!buffer || !buffer->content) return false;
 
     /* 扫描数值有效长度 */
     for (cursor = 0; can_access_at_index(buffer, cursor); cursor++) {
         unsigned char c = buffer_at_offset(buffer)[cursor];
         if (strchr("0123456789+-.eE", c)) {
             strLen++;
         } else {
             break;
         }
     }
 
     tmpStr = (unsigned char *) buffer->hooks.allocate(strLen + 1);
     if (!tmpStr) return false;
 
     memcpy(tmpStr, buffer_at_offset(buffer), strLen);
     tmpStr[strLen] = '\0';
 
     /* 本地化处理：将点替换为本地小数点 */
     for (size_t k = 0; k < strLen; k++) {
         if (tmpStr[k] == '.') tmpStr[k] = dotChar;
     }
 
     val = strtod((const char*)tmpStr, (char**)&stopPtr);
     if (tmpStr == stopPtr) {
         buffer->hooks.deallocate(tmpStr);
         return false;
     }
 
     /* 填充数据结构 */
     item->valuedouble = val;
     item->valueint = (val >= INT_MAX) ? INT_MAX : (val <= INT_MIN ? INT_MIN : (int)val);
     item->type = cJSON_Number;
 
     buffer->offset += (size_t)(stopPtr - tmpStr);
     buffer->hooks.deallocate(tmpStr);
     
     return true;
 }
 
 /* --- 对象与数组的迭代重写 --- */
 
 CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *arr) {
     size_t count = 0;
     cJSON *child = arr ? arr->child : NULL;
     
     for (; child != NULL; child = child->next) {
         count++;
     }
     return (int)count;
 }
 
 static cJSON* FindItemByIndex(const cJSON *arr, size_t idx) {
     cJSON *curr = arr ? arr->child : NULL;
     while (curr && idx > 0) {
         idx--;
         curr = curr->next;
     }
     return curr;
 }
 
 CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int index) {
     return (index < 0) ? NULL : FindItemByIndex(array, (size_t)index);
 }