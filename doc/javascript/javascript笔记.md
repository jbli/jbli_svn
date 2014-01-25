javascript
=====================
#变量
定义: var hello; 
赋值： hello = "hello";
例子： var age=10; age = -age;
#运算
var result = 10%2;
result++;
++result;

#判断
if
```
if (age > 18)
{
  alert("ok");
}else
{
  alert("err");
}

hello == "hello" ; 判断相等
```
switch-case   其中switch值可以是字符串。

```
var t = 2;
switch(t){
case 1:
   alert("1");
   break;
case3:
   alert("3");
   break;
 default:
   alert("end");
}
```
逻辑计算

- and: &&
- or: ||
- not: !

#循环
```
whiel(){
}
或
do{
}while()
或
for(i=0;i<5;i++){
}
```
跳出循环

- break
- continue

#函数
定义

```
function fun_name(){}
```
函数变量
var f = new Function("x","y", "return x*y") ;

等价于
function f(x,y) {return x*y}

一个例子

```
function add(a,b){
  return a+b;
}
function cal(f,a,b){
  return f(a,b);
}
print(cal(add,5, 6));
```

变量空间
函数外变量全局可见，函数内变量函数可见。

#数组
```
var marks = new Array();
marks[0] = 89;
marks[1] = 78;
marks[2] = 90;
```
###创建数组的几种方式
数组可以动态增长的，可以设一个size,初始化时分配大小；也可以赋多个会值初始化。下标从0开始。

```
var a = new Array();
var b = new Array(size);
var c = new Array(d1,d2,..,dn);
var d = [d1,d2,..,dn]
```
注意：当使用new Array(5)创建，只会认为是第二种方式，不会认为是第三种方式。

###数组长度
a.length给也数组a的长度，是其中最大下标+1；

###转换数组为字符串
```
var colors = ["red", "blue", "green"];
alert(colors.toString());
alert(colos.valueoOf());
alert(colors);
alert(colors.join(","));
alert(colors.join("||"));
```
###堆栈操作
```
var colors = new Array();
var count = colors.push("red", "green");
alert(count);
var item = colors.pop();
alert(item);
alert(colors.length);
```

###队列操作
```
var colors = new Array();
var count = colors.push("red", "green");
var item = colors.shift();
alert(item);
```
###排序操作
```
var values = [0, 1, 5, 10];
values.sort();  
values.reverse();
```
values.sort(); 默认从小到大排序，sort中也可以传递一个函数变量做为参数,函数对两个参数做比较。

```
function compare(v1, v2){
  if(v1<v2){
    return 1;
  }else if(v1>v2){
    return -1;
  }else{
    return 0;
  }
}

var values=[1, 3, 5];
values.sort(compare);
```

###其它操作
- 连接 concat

```
color2 = colors.concat("yellow", ["aa", "bb"]);
```

- 截取数组的一部分 slice

```
colors3 = colors.slice(1,4);
```
- splice(开始位置， 删除个数， 插入元素...)

```
删除: splice(0,2)
插入： splice(2,0,"red", "green")
替换： splice(2,1,"red", "green")
```

#对象
对象是把多个数据集中在一个变量中。对象是一个属性的集合，每个属性有自己的名字和值。

javaScript并不像其他OOP语言那样有类的概念，不是先设计类再制造对象。

###创建对象
可以用new,也可以用{}来初化对象。

```
var o = new Object();
var ciclr = {x:0, y:0, radius:2};
```

###访问对象属性

创建一个对象后，就可以任意加属性。可以给某个属性再创建一个对象，再加新属性。

```
var book = new Object();
book.title = "html5";
book.chapter1 = new Object();
book.chapter1.title = "html5.1";
```
###删除对象属性

```
delete book.chapter1;
或
book.chapter1 = null;
```
###遍历对象属性
```
for(var x in o){
  alert(x,o[x])
}
```

###构造方法
1. 不直接制造对角
2. 通过this来定义成员
3. 没有return

```
function Rect(w,h){
  this.width=w;
  this.height =h;
  this.area = function(){ return this.width *              this.height;
   }
}

var r = new Rect(5,10); alert(r.area());
```

###原型对象
- 对象的prototype属性指定了它的原型对象，可以用运算符直接读它的原型对象的属性
- 当写这个属笥时才在它自己内部产生实际的属性。也就是对属性赋值。

例子：

```
function Person(){}
Person.prototype.name = "aabb";
Person.prototype.age = 29;
Person.prototype.job = "software Engineer";
Person.prototype.sayName = function(){
  alert(this.name);
};
var person1 = new Person();
person1.sayName(); //"aabb"
var person2 = new Person();
person2.sayName();//"aabb"
alert(person1.sayName == person2.sayName); //true

person1.name = "ccdd"; //重新赋值
alert(person1.name); //"ccdd" from instance
alert(person2.name); //"aabb" from prototpye

```
原型存在的问题，修改一个实例的原型的值 。


#输出

- print("aabb");
- document.write("hello world\n");
- alert("ok"); 弹出小对话框


