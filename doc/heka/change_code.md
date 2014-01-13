修改源码
========================
- 实现功能，第一次读文件时，从尾部读取
在/heka/plugins/file/logfile_input.go函数func (fm *FileMonitor) Init(conf *LogfileInputConfig) (err error)中添加如下代码

```
    //fm.seek = 0
    var fd *os.File
    if fd, err = os.Open(file); err != nil {
       return
    }
    defer fd.Close()
    fm.seek, _ = fd.Seek(0, 2)
        fm.fd = nil
```


