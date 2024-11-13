#include <stdio.h>
#include <git2.h>
#include <unistd.h>

void libgit_error_check(int error)
{
    if (error < 0)
    {
        const git_error *e = git_error_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
        exit(error);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Missing filename\n");
        return 1;
    }
    const char *filename = argv[1];

    // libgit initialization and looking if there is repository in given localization
    git_libgit2_init();
    git_repository *repo = NULL;
    int error = git_repository_open_ext(&repo, filename, 0, NULL);
    libgit_error_check(error);


    //// testing stuff out
    git_oid commit_oid;
    error = git_reference_name_to_id(&commit_oid, repo, "HEAD");
    libgit_error_check(error);

    git_commit *commit = NULL;
    error = git_commit_lookup(&commit, repo, &commit_oid);
    libgit_error_check(error);

    const char *message = git_commit_message(commit);
    printf("Commit: %s\nMessage: %s\n", git_oid_tostr_s(&commit_oid), message);

    unsigned int count = git_commit_parentcount(commit);
    for (unsigned int i=0; i<count; i++) {
        const git_oid *nth_parent_id = git_commit_parent_id(commit, i);

        git_commit *nth_parent = NULL;
        int error = git_commit_parent(&nth_parent, commit, i);
        
        message = git_commit_message(nth_parent);
        printf("Parent %d\nCommit: %s\nMessage: %s\n",i+1 , git_oid_tostr_s(nth_parent_id), message);

    }

    //// end of testing

    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0;
}
