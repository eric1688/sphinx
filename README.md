# sphinx
Sphinx for Chinese
网上的sphinx中文版（http://code.google.com/p/sphinx-for-chinese/）比较旧，所以根据需要改写
sphinx 2.2.9版本， 根据sphinx for chinese改写到sphinx2.2.9版本

sphinx-for-chinese安装
解压 
$ git clone https://github.com/eric1688/sphinx
$ cd sphinx

编译（假设安装到/usr/local/sphinx目录，下文同）
$ ./configure --prefix=/usr/local/sphinx
--prefix 指定安装路径
--with-mysql 编译mysql支持
--with-pgsql 编译pgsql支持
$ make
$ make install

配置中文支持

$ tar -xvf xdict_1.1.tar.gz
$ /usr/local/sphinx/bin/mkdict xdict_1.1.txt xdict #从xdict_1.1.txt生成xdict文件，xdict_1.1.txt文件可以根据需要进行修改
$ cp xdict /usr/local/sphinx/etc/

修改sphinx.conf索引配置文件

在索引配置项中添加以下两项
charset_type = utf-8
chinese_dictionary = /usr/local/sphinx/etc/xdict

注意在source部分一定加上 
sql_query_pre = SET NAMES utf8
否则中文分词无法起作用。

至此，完成中文支持配置。