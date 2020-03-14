TEMPLATE = subdirs

SUBDIRS = \
    app \
    common \
    db \
    savefile

app.depends += \
    common \
    db \
    savefile

db.depends += \
    common

savefile.depends += \
    common \
    db
