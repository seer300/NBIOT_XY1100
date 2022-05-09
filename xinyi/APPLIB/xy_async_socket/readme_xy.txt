xy_async_socket_api目录中的代码提供异步socket套接字编程接口，异步socket表现在TCP connect时候不会阻塞，直接返回成功，
connect结果、下行数据接收，发送上行数据结果以及异常事件通过回调方式通知上层注册的应用

注意：
1、回调事件处理最好发送到各自应用的主线程处理，不要在回调函数中直接再次调用socket api接口
2、xy_async_socket_api功能如果不需要，可以通过修改编译宏，使代码不编译，编译宏在 TARGETS\xinyiNBSoC\GCC-ARM\make\feature.mk中XY_ASYNC_SOCKET_SUPPORT，
该值为y表示参与编译，否则不编译
3、异步socket套接字编程接口使用demo参见 TARGETS\xinyiNBSoC\USERAPP\examples\xy_socket_demo\xy_async_socket_demo.c文件
