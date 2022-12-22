# boTARdat
-----

## TODO:
- Choosing libraries
  - [x] Find a hash library (uthash)
  - [x] Find an archive library (libarchive)
- Directory Data Structure
  - [x] Design and implement node struct
  - Implement basic functionality
    - [x] Create node
    - [x] Destroy node
    - [x] Find node by path
    - [x] Add node to parent
    - [x] Remove node from parent
    - [ ] Temporary files for write
- Archive IO
  - [x] Create structure from archive
  - [ ] Recreate archive from structure
- FUSE API
  - Readonly
    - [ ] getattr
    - [ ] read
    - [ ] readdir
    - [ ] lseek
  - Write operations
    - [ ] write
    - [ ] mknod
    - [ ] mkdir
    - [ ] truncate
    - [ ] chmod
    - [ ] chow
    - [ ] utime
    - [ ] rmdir
    - [ ] rename
  - Links support
    - [ ] link
    - [ ] symlink
    - [ ] readlink
    - [ ] unlink
- Utility
  - [x] Bash script for easier build
- User Experience
  - [ ] Help prompt
  - [x] Come up with a meaning for boTARdat
