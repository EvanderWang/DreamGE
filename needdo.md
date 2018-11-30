一：构建程序基本框架目标
    1.程序描述运行时关系，类似system，但须基于带时间的数据（Observerable）
    2.包管理，一个包实现一套接口，同时要有测试内容的管理、以及运行时错误的自动添加测试（mock？）。
二：基本库构建
    1.基于ISPC的可并行数学库
    2.跨平台的渲染器（output）
    3.跨平台的输入模块（input）

Thinking:
    1.每一个方法都相当于一个operator？ operator需要接口，以便更抽象的operator可以实现。
    2.传入operator的operator操作对象是observerable，而传入operator的其他内容是observerable的一个时刻的数据，该如何组合？（builder与subsystem的组合）
    3.使用combain_latest 和 sample（需自行实现）
    4.弄清楚对象生存周期