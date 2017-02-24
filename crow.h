#define SIZE 1024

struct string{
    char *ptr;
    size_t len;
};

struct user {
    char* id;
    char* username;
    char* discriminator;
    char* avatar;
    int bot;
    int mfa_enabled;
    int verified;
    char* email;
};

struct message {
    char* id;
    char* channel_id;
    struct user author;
    char* content;
    char* timestamp;
    char* edited_timestamp;
    int tts;
    int mention_everyone;
    /*
    TODO:
    mentions
    mention_roles
    attachments
    embeds
    nonce
    */
    struct reaction reactions[];
    int pinned;
    char* webhook_id;
};
struct emoji {
    char* id;
    char* name;
};
struct reaction {
    int counter;
    int me;
    struct emoji _emoji;
};
