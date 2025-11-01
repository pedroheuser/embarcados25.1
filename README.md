# IoT RGB Lighting Monorepo

Monorepo para o projeto de IoT focado em monitoramento e controle de iluminação RGB.

- `backend`: API construída com Django e Django REST Framework.
- `frontend`: Aplicativo mobile em React Native com integração à API via Axios.

## Backend

Projeto Django (`config`) com aplicação `api` que expõe endpoints REST para status, leituras de luminosidade e controle manual. Inclui configuração de CORS liberada para facilitar o desenvolvimento com o aplicativo mobile.

## Frontend

Aplicativo React Native que consome os endpoints do backend para exibir o status do sistema, última leitura de luminosidade e enviar comandos manuais para o controlador RGB.

