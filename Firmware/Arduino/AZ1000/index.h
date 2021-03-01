const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="ko">
    <head>
        <title>AV1000 Timing Generate</title>
        <meta charset="utf-8" name="viewport" content="width=device-width, initial-scale=1">
        <style type="text/css">
            body { background-color: lightblue; }

            table {
                width: 500px;
                height: 100px;
            }

            tr td{
                text-align: center;
            }
        </style>
        <script src="https://code.jquery.com/jquery-3.4.1.min.js"></script>
        <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.2/css/bootstrap.min.css">
    </head>

    <body>
        <center>
        <h2>GUI Programming Guide</h2>
        <br>
        <table>
            <tr>
                <td style="width:110px; text-align:right">
                    <label><input type="checkbox" id="Init" value="1"/>Initial</lable>
                </td>
                <td>
                    <label for="Addr">Addr</label><input type="number" id="Addr" min="0" max="8" step="1" value="0"/>
                </td>
                <td>
                    <label for="Data">Data</label><input type="number" id="Data" min="0" max="64" step="1" value="0"/>
                </td>
                <td>
                    <label><input type="checkbox" id="Reset" value="2"/>Reset</lable>
                </td>
                <td>
                    <input type="button" class="btn btn-primary btn-xs" id="Send" value="Send">
                </td>
            </tr>
            <tr>
                <form action="/uploadFile" method="POST" enctype="multipart/form-data">
                <td>
                </td>
                <td colspan="3">
                    <input type="file" id="fileToLoad" name="datafile" size="40" accept=".txt">
                </td>
                <td>
                    <input type="submit" class="btn btn-primary btn-xs" value="Load">
                </td>
                </form>
            </tr>
        </table>
        <br>

        <textarea id="console" cols="60" rows="10" style="display:block; resize:none"></textarea>
        </center>


        <script language="javascript">
            $(function() {              // $(document).ready(function() {
            });

            $('#Send').on('click', function(e) {
                var str = 'Send?';
                var msg = '';
                var nAddr = $('#Addr').val();
                var nData = $('#Data').val();
                var ctrl = 0;
//                if( $('input:checkbox[id="Init"]').is(":checked") == true ) {
                    ctrl |= parseInt($('input:checkbox[id="Init"]:checked').val());
//                }
//                if( $('input:checkbox[id="Reset"]').is(":checked") == true ) {
                    ctrl |= parseInt($('input:checkbox[id="Reset"]:checked').val());
//                }
                str += 'Ctrl=' + ctrl.toString();
                str += '&Addr=' + nAddr;//$('#Addr').val();
                str += '&Data=' + nData;//$('#Data').val();

                var xhr = new XMLHttpRequest();
                xhr.open('GET', window.location.href+str, true);
                xhr.send();
                xhr.onreadystatechange=function() {
                    if( xhr.readyState == 4 && xhr.status == 204 ) {
                        if( ctrl & 0x1 ) msg += 'Initial ';

//                        if( parseInt($('#Addr').val()) != 0 ) {
                        if( parseInt(nAddr) != 0 ) {
                            msg += `Packet2(${nAddr}, ${nData}) `;
//                            msg += 'Packet2(' + nAddr + ', ' + nData + ') ';
                        } else {
                            if( parseInt(nData) != 0 ) {
                                msg += `Packet1(${nData}) `;
//                                msg += 'Packet1('+$('#Data').val() + ') ';
                            }
                        }

                        if( ctrl & 0x2 ) msg += 'Reset ';

                        if( msg != '' ) dbgout(msg + 'cycle\n');
                    }
                }
            });

            $('#Addr').on('change', function(e) {
            });

            $('#Data').on('change', function(e) {
            });

            $(document).on('click', '#console', function(e) {
                $('#console').scrollTop( $('#console')[0].scrollHeight );
            });

            function dbgout(s) {
                $('#console').val( $('#console').val() + s);
//                $('#console').scrollTop( $('#console')[0].scrollHeight );
            }

        </script>
    </body>
</html>
)rawliteral";

