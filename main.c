#include <stdio.h>
#include <git2.h>
#include <unistd.h>

int is_file_in_repository(const char *filepath) {
    return 1;
}

void libgit_error_check(int error){
    if (error < 0) {
    const git_error *e = git_error_last();
    printf("Error %d/%d: %s\n", error, e->klass, e->message);
    exit(error);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2){
        fprintf(stderr, "Missing filename\n");
        return 1;
    }
    const char* filename = argv[1];

    git_libgit2_init();
    git_repository *repo = NULL;
    int error = git_repository_open_ext(&repo, filename, 0, NULL);
    libgit_error_check(error);



    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0;
}
