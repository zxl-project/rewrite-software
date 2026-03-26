/* * JsonEngine 演示程序 - 基于重构后的引擎构建
 * 展示了对象创建、预分配内存打印以及结构化数据的序列化。
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "cJSON.h" /* 假设你已按照之前的建议重命名或保留此头文件 */
 
 /* 重新定义演示用数据结构 */
 typedef struct {
     const char *tag;
     double coordinates[2]; /* 0: lat, 1: lon */
     const char *location_details[5]; /* addr, city, state, zip, country */
 } GeoRecord;
 
 /* * 核心演示函数：安全打印到预分配缓冲区
  * 逻辑重写：采用更加严谨的边界检查和流程控制
  */
 static int DemonstrateBufferSafety(JsonNode *node_to_print) {
     char *standard_output = NULL;
     char *safe_buffer = NULL;
     char *fail_buffer = NULL;
     size_t required_sz = 0;
 
     /* 获取标准打印结果用于对比 */
     standard_output = Json_Stringify(node_to_print);
     if (!standard_output) return -1;
 
     required_sz = strlen(standard_output);
 
     /* 准备足够的空间 (增加冗余偏移以确保安全) */
     size_t success_sz = required_sz + 10;
     safe_buffer = (char*)malloc(success_sz);
     
     /* 准备不足的空间 (触发失败测试) */
     size_t deficit_sz = required_sz > 5 ? required_sz - 5 : 1;
     fail_buffer = (char*)malloc(deficit_sz);
 
     if (!safe_buffer || !fail_buffer) {
         fprintf(stderr, "Critical: Memory allocation failed in demo.\n");
         free(standard_output);
         exit(EXIT_FAILURE);
     }
 
     /* 执行预分配打印 */
     if (!Json_StringifyStatic(node_to_print, safe_buffer, (int)success_sz, 1)) {
         printf("Error: Static stringify failed unexpectedly!\n");
     } else {
         printf("--- Success Output ---\n%s\n", safe_buffer);
     }
 
     /* 测试防御性边界：故意传入过小的缓冲区 */
     printf("Testing overflow protection... ");
     if (Json_StringifyStatic(node_to_print, fail_buffer, (int)deficit_sz, 1)) {
         printf("FAILED (Buffer should have been too small)\n");
     } else {
         printf("PASSED (Caught insufficient memory)\n");
     }
 
     /* 清理 */
     free(standard_output);
     free(safe_buffer);
     free(fail_buffer);
     return 0;
 }
 
 /* * 构建复杂的 JSON 结构
  * 结构重写：将原版杂乱的创建逻辑拆分为清晰的业务块
  */
 static void RunComplexObjectDemo(void) {
     JsonNode *root = NULL;
     JsonNode *sub_group = NULL;
     int i, j;
 
     /* 1. 构建类似视频元数据的对象 */
     root = Json_NewObject();
     Json_LinkToObject(root, "identifier", Json_NewString("Demo_Node_001"));
     
     sub_group = Json_NewObject();
     Json_LinkToObject(root, "config", sub_group);
     Json_LinkToObject(sub_group, "resolution", Json_NewString("4K"));
     Json_LinkToObject(sub_group, "bit_rate", Json_NewNumber(50000));
     Json_LinkToObject(sub_group, "is_active", Json_NewTrue());
 
     DemonstrateBufferSafety(root);
     Json_DeleteNode(root);
 
     /* 2. 构建二维数组 (矩阵) */
     int matrix[2][2] = {{1, 0}, {0, 1}};
     root = Json_NewArray();
     for (i = 0; i < 2; i++) {
         JsonNode *row = Json_NewArray();
         for (j = 0; j < 2; j++) {
             Json_LinkToArray(row, Json_NewNumber(matrix[i][j]));
         }
         Json_LinkToArray(root, row);
     }
     
     printf("\nIdentity Matrix:\n");
     DemonstrateBufferSafety(root);
     Json_DeleteNode(root);
 
     /* 3. 构建地理信息记录 */
     GeoRecord samples[1] = {
         { "Home", {39.9, 116.4}, {"Road 1", "Beijing", "BJ", "100000", "CN"} }
     };
 
     root = Json_NewArray();
     for (i = 0; i < 1; i++) {
         JsonNode *entry = Json_NewObject();
         Json_LinkToObject(entry, "label", Json_NewString(samples[i].tag));
         Json_LinkToObject(entry, "lat", Json_NewNumber(samples[i].coordinates[0]));
         Json_LinkToObject(entry, "lon", Json_NewNumber(samples[i].coordinates[1]));
         
         JsonNode *details = Json_NewObject();
         Json_LinkToObject(entry, "meta", details);
         Json_LinkToObject(details, "city", Json_NewString(samples[i].location_details[1]));
         Json_LinkToObject(details, "country", Json_NewString(samples[i].location_details[4]));
         
         Json_LinkToArray(root, entry);
     }
 
     printf("\nGeo Records:\n");
     DemonstrateBufferSafety(root);
     Json_DeleteNode(root);
 }
 
 int main(void) {
     /* 显示版本信息 */
     printf("JSON Engine Active. Version: %s\n", Json_GetVersion());
     printf("==========================================\n");
 
     /* 启动演示 */
     RunComplexObjectDemo();
 
     return 0;
 }