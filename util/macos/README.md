## png2icns 
一个简单的把png文件转为icns的小工具，直接在命令行中，输入  

```shell
png2icns <target png file>
```

即可。

## 制作dmg
出处为[create-dmg](https://github.com/create-dmg/create-dmg)

制作dmg需要用create-dmg，这个在brew中可以获取到。具体的命令例子如下：  

```shell
#!/bin/sh
test -f Application-Installer.dmg && rm Application-Installer.dmg
create-dmg \
        --volname "Application Installer" \
        --volicon "logo.icns" \
        --background "installer_background.png" \
        --window-pos 200 120 \
        --window-size 800 400 \
        --icon-size 100 \
        --icon "Application.app" 200 190 \
        --hide-extension "Application.app" \
        --app-drop-link 600 185 \
        "Application-Installer.dmg" \
        "source_folder/"
```

为了方便自己使用，我自己随便复制了一个。