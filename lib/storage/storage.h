#pragma once


class Storage {

    public:
        Storage();

        bool creds_already_exist(char *, char *);
        void store_creds(char *, char *);
        void clear_creds();

};

extern Storage storage;