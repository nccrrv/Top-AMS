# Top-AMS
- Q群:104103478九,注明来意
# **施工中**



### 代办
***
- 基本使用说明(放Readme.md)
  - 教程,编译环境
- 自动续料
    - 思路:默认会续当前通道的下一个通道的料,在切片时候就要注意好.软件架构上,维护一个长度为使用的通道数(通过是否是NC脚判断)的布尔向量,记忆当前通道是否被续料过,这个记忆状态会在本次打印任务结束后重置
- 异常处理
  - mqtt断开
  - 因为料线刚好没了,无法退线的的报错
- 其他
  - 测试sdkconfg的其他编译选项能否缩小bin大小
  - 整理一下代码,看下能否缩小二进制大小 
    - 排除不必要的头文件,换个IO
  
