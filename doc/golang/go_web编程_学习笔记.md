《Go Web基础》学习笔记
----
设置环境变量

```
export GOPATH=/Users/lijinbang/mywork/godir201402/golibs
export PATH=$PATH:/Users/lijinbang/mywork/godir201402/golibs/bin
```
下载beego和bee

```
git get github.com/astaxie/beego
git get github.com/astaxie/beego/bee
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


资料
----
[Unknown/go-web-foundation](https://github.com/Unknwon/go-web-foundation)