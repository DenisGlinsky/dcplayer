Да, это нормально. Ошибки объясняются просто:
  • No such remote 'origin' — у локального репозитория ещё нет remote с именем origin.
  • src refspec main does not match any — у вас ещё нет локальной ветки main с хотя бы одним коммитом.

Раз SSH уже работает, дальше всё сводится к правильной последовательности.

Что можно сделать уже сейчас

Если хотите просто подготовить локальный репозиторий к будущему push:

cd ~/Documents/dcplayer
git init
git branch -M main
git remote add origin git@github.com:DenisGlinsky/dcplayer.git
git remote -v

После этого remote будет настроен, но пушить всё равно пока нечего — и это нормально.

Когда будете готовы отправить первый код

cd ~/Documents/dcplayer
git add .
git commit -m "Initial scaffold"
git push -u origin main

Если origin уже потом появится, а вы снова захотите поменять URL

Тогда используется не add, а set-url:

git remote set-url origin git@github.com:DenisGlinsky/dcplayer.git

Важная проверка перед первым push

Если на GitHub-репозитории уже есть README, license или любой первый коммит, тогда перед push лучше сделать:

git fetch origin
git pull --rebase origin main

А потом уже:

git push -u origin main

Если репозиторий на GitHub пустой, это не нужно.

Самая короткая правильная схема для вас сейчас

cd ~/Documents/dcplayer
git init
git branch -M main
git remote add origin git@github.com:DenisGlinsky/dcplayer.git

И позже, когда появится что коммитить:

git add .
git commit -m "Initial scaffold"
git push -u origin main