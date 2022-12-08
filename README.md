# DB_System_Architecture_P2
Repository for the development of the second project of the course DATABASE SYSTEM ARCHITECTURE (INFO-H-417) from the program BDMA. Fall 2022.

This project consists of developing an extension of Postgres, providing the URL datatype (equivalent to the Java datatype java.net.URL).

Group members:

- [Aliakberova, Liliia](https://github.com/Liliia-Aliakberova)
- [Gepalova, Arina](https://github.com/omymble)
- [Lorencio Abril, Jose Antonio](https://github.com/Lorenc1o)
- [Mayorga Llano, Mariana](https://github.com/marianamllano)

Professor: Sakr, Mahmoud

## Instructions

This PostgreSQL extension has nothing very special, the usual steps need to be performed.

If you want to create the extension for one user, perform the following:

$ sudo make install
$ psql [user] -c "CREATE EXTENSION url;"

We have written some common test that one could be interested in performing with this extension, you can execute them by running:

$ psql [user] -a -f Test/test_url.sql

## Code

The code is organized as usual for PostgreSQL extensions:
- url--0-0-1.sql is the SQL script where the extension is defined, interfacing between PostgreSQL and C
- url.control is a control file with basic configuration parameters
- Makefile is the make orchestrator to easily link all dependencies and compile as required
- url.c is the C file where the functions defined for the extension are implemented
- In the Test directory you can find the test_url.sql script, where all functionality is checked to work and results.txt, where you can see sample results obtained running the test script