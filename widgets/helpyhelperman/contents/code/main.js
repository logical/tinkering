 
layout = new LinearLayout(plasmoid);
layout.orientation=QtVertical; 
var textentry =new LineEdit();
layout.addItem(textentry);
textentry.clearButtonShown=true;

var links=new TextEdit();
links.readOnly=true;
layout.addItem(links);
var engine = dataEngine("executable");    


function updatelist(source,data)
{
  output=data["stdout"];
  plasmoid.resize(500,30);
  links.text='';
  if(output){
    links.text=output;
    plasmoid.resize(500,700);
  }
  engine.disconnectSource(source,this);
}

function openhelp(){
  arg=["man:"+textentry.text];
  plasmoid.runCommand("khelpcenter",arg);
  
}

function showdata(text){
  command="man -k "+text;
//  print(command);
  engine.connectSource(command, updatelist);
  
}
textentry.textChanged.connect(showdata);
textentry.returnPressed.connect(openhelp);

