REM 
REM Use gzip to compress static files
REM 
call gzip    Tool html\css  bootstrap_min_css              css
call gzip    Tool html      Custom                         css
call gzip    Tool html\js   bootstrap_min_js               js
call gzip    Tool html\js   events                         js
call gzip    Tool html\js   jquery_min_js                  js
call gzip    Tool html\js   tether_min_js                  js
call gzip    Tool html\js   RGraphCC                       js
call gzip    Tool html\js   RGraphCE                       js
call gzip    Tool html\js   RGraphLi                       js
REM 
REM Convert the compressed files
REM Copy the .c/.h files into "Generated" folder
REM Delete the temp files from HTML folder
REM 
call cc_gzip Tool html\css       bootstrap_min_css              gz
call cc_gzip Tool html           Custom                         gz
call cc_gzip Tool html\js        bootstrap_min_js               gz
call cc_gzip Tool html\js        events                         gz
call cc_gzip Tool html\js        jquery_min_js                  gz
call cc_gzip Tool html\js        tether_min_js                  gz
call cc_gzip Tool html\js        RGraphCC                       gz
call cc_gzip Tool html\js        RGraphCE                       gz
call cc_gzip Tool html\js        RGraphLi                       gz
REM 
REM Convert the non-static source code files, images, ...
REM Copy the .c/.h files into "Generated" folder
REM Delete the temp files from HTML folder
REM 
REM
REM htm files
REM
call cc      Tool html           Authen                         htm
call cc      Tool html           Error404                       htm
call cc      Tool html           FormGET                        htm
call cc      Tool html           FormPOST                       htm
call cc      Tool html           index                          htm
call cc      Tool html           IPConfig                       htm
call cc      Tool html           OSInfo                         htm
call cc      Tool html           Products                       htm
call cc      Tool html           Samples                        htm
call cc      Tool html           Shares                         htm
call cc      Tool html           VirtFile                       htm
REM
REM ico files
REM
call cc      Tool html           favicon                        ico
REM
REM png files
REM
call cc      Tool html           emWeb_Logo                     png
REM
REM gif files
REM
call cc      Tool html           emNet_Icon                     gif
call cc      Tool html           embOSMPU_Icon                  gif
call cc      Tool html           emFile_Icon                    gif
call cc      Tool html           emLoad_Icon                    gif
call cc      Tool html           Empty                          gif
call cc      Tool html           emSSH_Icon                     gif
call cc      Tool html           emSSL_Icon                     gif
call cc      Tool html           emUSBD_Icon                    gif
call cc      Tool html           emUSBH_Icon                    gif
call cc      Tool html           emWin_Icon                     gif
call cc      Tool html           GreenRUp                       gif
call cc      Tool html           Logo                           gif
call cc      Tool html           RedRDown                       gif
call cc      Tool html           WhiteR                         gif

  