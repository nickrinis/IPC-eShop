<h1 align="center">IPC-eShop</h1>
<p align="justify"><strong>IPC-eShop is a programme built using Inter-Process Communication (IPC) in C programming language. It simmulates an e-shop that allows the user to browse, reserve books wherever possible and proceed to checkout.</strong>
<br/>
<h2>About</h2>
This project was part of my coursework regarding Linux environment programming, with emphasis on the use of pipes and forks.

<h2>Installation</h2>
Requirements: Linux Operating System, C compiler

1. Clone the repository: git clone https://github.com/nickrinis/IPC-eShop.git
2. Navigate to the project directory: cd IPC-eShop
3. Compile the source code: make
4. Run the application: ./eshop_server

<h2>Usage</h2>

The system uses sockets to connect the server and client pipes, with each having different functionalities.

Once connected through the socket, the client is able to make requests for purchasing a product based on availability.

The server reads the request from the client after a connection is established, and if the products are available, informs the client of the charge. If the products are unavailable the client is informed once again. Once all the requests are complete, the system exports a report where details for each product is included, for example, product description, demand, ammount sold, total income, etc.

<h2>Credits</h2>

IPC_eShop relies on the following libraries and tools:
- string
- sys/socket
- sys/un
- unistd
- errno
- time
- fcntl
- sys/wait
- sys/types
- arpa/inet
- netinet/in
- stdint

<h2>Contact</h2>
If you have any questions or feedback, please contact me at [nickmarinis12@hotmail.gr](mailto:nickmarinis12@hotmail.gr).

<h2>Copyright</h2>
This project is licensed under the terms of the MIT license.
