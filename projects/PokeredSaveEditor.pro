TEMPLATE = subdirs

SUBDIRS = \
    common \
    db \
    savefile \
    core \
    app

db.depends = common
savefile.depends = common db
core.depends = common db savefile
app.depends = common db savefile core
