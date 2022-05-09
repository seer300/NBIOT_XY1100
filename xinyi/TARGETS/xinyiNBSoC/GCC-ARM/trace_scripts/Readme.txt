为了省去Trace32脚本使用前繁杂的脚本修改工作，新增Mv_DumpFile_To_Trace.bat脚本工具。
在特定的死机文件路径下，运行此脚本，即可自动修改Trace32脚本。 

使用方法：

1. 先将dump文件和axf/raw binary 放在同一路径下。

2. 将Mv_DumpFile_To_Trace.bat 拷贝至上述文件路径

3. 使用Notepad++打开Mv_DumpFile_To_Trace.bat，并修改其中的src_dir变量为实际dump的文件路径

4.双击脚本运行

5.打开Trace选择对应的M3或者DSP脚本，点击Do后，即可解析死机。


