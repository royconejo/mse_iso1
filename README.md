# Implementacion de Sistemas Operativos.
## MSE 4ta cohorte, 2019.
## Santiago Germino.

### Compilación

Desde directorio raíz del repositorio:

```shell
$ cd projects/iso-examen
$ make
```

Para compilar y subir firmware a la EDU-CIAA-NXP:

```shell
$ make flash
```

### Uso

Lo botones **TEC_1** y **TEC_2** de la EDU-CIAA-NXP corresponden a las señales **B1** y **B2** respectivamente. Los parametros de la UART son 9600-8-N-1.

### Limitaciones

* El path absoluto donde se aloje este proyecto *no debe contener espacios*. Esto quizá se resuelva en futuras versiones.
* El sistema de build no tiene intencion de soportar Windows, pero se podría intentar utilizar un entorno como [MSYS](http://www.mingw.org/wiki/MSYS).

### Implementación

Se aprovechó la cursada para desarrollar un SO para el proyecto final de la Especialización en Sistemas Embebidos (CESE).
Este código es parte del proyecto [RETRO-CIAA](http://www.retro-ciaa.com) y aún se encuentra en desarrollo.

*EOF*
