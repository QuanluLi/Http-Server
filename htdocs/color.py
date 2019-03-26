#!/usr/bin/python
# -*- coding: UTF-8 -*-

# filename：test.py

# CGI处理模块
import cgi, cgitb 

# 创建 FieldStorage 的实例化
form = cgi.FieldStorage() 

# 获取数据
site_name = form.getvalue('color')

print "Content-type:text/html"
print
print "<html>"
print "<head>"
print "<meta charset=\"utf-8\">"
print "<title>%s</title>" % site_name
print "</head>"
print "<body bgcolor=%s>" % site_name
print "<h1>this is %s</h1>" % (site_name)
print "</body>"
print "</html>"
