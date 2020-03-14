TEMPLATE = subdirs

SUBDIRS = \
    common \
    db \
    savefile \
    app

db.depends = common
savefile.depends = common db
app.depends = common db savefile
