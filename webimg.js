console.log('webimg.js loaded !')
x = [1,2,3,4,5]
y = x.map(function(xi,i){
    var li = document.createElement('li')
    var y=xi*3
    li.textContent=i+') '+y
    document.body.appendChild(li)
    return y
})

