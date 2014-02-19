《Go Web基础》学习笔记
----
设置环境变量

```
export GOROOT=/data/go-dev-201402/go  
export GOPATH=/data/go-dev-201402/golibs 
export PATH=$PATH:$GOROOT/bin:$GOPATH/bin
```
下载beego和bee

```
go get github.com/astaxie/beego
go get github.com/beego/bee
```
创建一个简单示例

```
cd $GOPATH/src
bee new myapp
cd $GOPATH/src/myapp
bee run myapp

```

做一个修改$GOPATH/src/myapp/controllers/default.go

```
func (this *MainController) Get() {
	/*
		this.Data["Website"] = "beego.me"
		this.Data["Email"] = "astaxie@gmail.com"
		this.TplNames = "index.tpl"
	*/
	this.Ctx.WriteString("appname: " + beego.AppConfig.String("appname") + "\nhttpport: " + beego.AppConfig.String("httpport") + "\nrunmode: " + beego.AppConfig.String("runmode"))
}
```
等价于

```
hp := strconv.Itoa(beego.HttpPort)
	this.Ctx.WriteString("\n\nappname: " + beego.AppName + "\nhttpport: " + hp + "\nrunmode: " + beego.RunMode)
```

日志记录方式

```
	beego.Trace("trace test")
	beego.Info("info test")
	beego.SetLevel(beego.LevelInfo)
	beego.Trace("trace test")
	beego.Info("info test")
}
```
go模板
======
if

```
             <div>
			  	{{if .TrueCond}}
			  	true condition.
			  	{{end}}
			  </div>

			  <div>
			  	{{if .FalseCond}}
			  	{{else}}
			  	false condition.
			  	{{end}}
			  </div>
```
输出结构体元素

```
type u struct {
		Name string
		Age  int
		Sex  string
	}
	user := u{
		Name: "Unknown",
		Age:  20,
		Sex:  "Male",
	}

	this.Data["User"] = user
```

```

			  <div>
			  	with output:
			  	{{with .User}}
				Name:{{.Name}}; Age:{{.Age}}; Sex:{{.Sex}}
			  	{{end}}
			  </div>
```
循环

```
nums := []int{1, 2, 3, 4, 5, 6, 7, 8, 9, 0}
	this.Data["Nums"] = nums
```
```
			  <div>
			  	range ouput:
			  	{{range .Nums}}
			  	{{.}}
			  	{{end}};

			  	{{range .Users}}
				Name:{{.Name}}; Age:{{.Age}}; Sex:{{.Sex}}
			  	{{end}}
			  </div>
```

模板变量的使用

```
this.Data["TplVar"] = "hey guys"
```
```
{{$tplVar := .TplVar}}
The template variable is: {{$tplVar}}.
```

模板中函数的使用

```
this.Data["Html"] = "<div>hello beego</div>"
	this.Data["Pipe"] = "<div>This string will be escaped through pipeline and template function</div>"
```
```
The result of tempalte function is: {{htmlquote .Html}}
pipeline: {{.Pipe | htmlquote}}
```
定义一个模板，多处使用

```
{{define "nested"}}
Nested template test
{{end}}
```

```
{{template "nested"}}
```


orm
==========
安装

```
sudo apt-get install pkg-config
sudo apt-get install sqlite3  
sudo apt-get install libsqlite3-dev

go get github.com/mattn/go-sqlite3
```
##一个例子
```
package main

import (
        "os"
        "path"
        "time"

        "github.com/Unknwon/com"
        "github.com/astaxie/beego/orm"
        _ "github.com/mattn/go-sqlite3"
)

const (
        // 设置数据库路径
        _DB_NAME = "data/test.db"
        // 设置数据库名称
        _SQLITE3_DRIVER = "sqlite3"
)

// 分类
type Category struct {
        Id              int64
        Title           string
        Created         time.Time `orm:"index"`
        Views           int64     `orm:"index"`
        TopicTime       time.Time `orm:"index"`
        TopicCount      int64
        TopicLastUserId int64
}

// 文章
type Topic struct {
        Id              int64
        Uid             int64
        Title           string
        Content         string `orm:"size(5000)"`
        Attachment      string
        Created         time.Time `orm:"index"`
        Updated         time.Time `orm:"index"`
        Views           int64     `orm:"index"`
        Author          string
        ReplyTime       time.Time `orm:"index"`
        ReplyCount      int64
        ReplyLastUserId int64
}

func RegisterDB() {
        // 检查数据库文件
        if !com.IsExist(_DB_NAME) {
                os.MkdirAll(path.Dir(_DB_NAME), os.ModePerm)
                os.Create(_DB_NAME)
        }

        // 注册模型
        orm.RegisterModel(new(Category), new(Topic))
        // 注册驱动（“sqlite3” 属于默认注册，此处代码可省略）
        orm.RegisterDriver(_SQLITE3_DRIVER, orm.DR_Sqlite)
        // 注册默认数据库
        orm.RegisterDataBase("default", _SQLITE3_DRIVER, _DB_NAME, 10)
}

func init() {
        // 注册数据库
        RegisterDB()
}

func main() {
        // 开启 ORM 调试模式
        orm.Debug = true
        // 自动建表
        orm.RunSyncdb("default", false, true)
}
```

资料
----
[Unknown/go-web-foundation](https://github.com/Unknwon/go-web-foundation)