import Vue from 'vue'
import App from './App.vue'

import { MdButton, MdToolbar, MdIcon, MdDrawer, MdField, MdSubheader, MdDivider, MdProgress, MdSnackbar, MdDialog, MdList, MdSwitch } from 'vue-material/dist/components'
import 'vue-material/dist/vue-material.min.css'

Vue.use(MdButton)
Vue.use(MdToolbar)
Vue.use(MdIcon)
Vue.use(MdDrawer)
Vue.use(MdField)
Vue.use(MdSubheader)
Vue.use(MdDivider)
Vue.use(MdProgress)
Vue.use(MdSnackbar)
Vue.use(MdDialog)
Vue.use(MdList)
Vue.use(MdSwitch)

Vue.config.productionTip = false

new Vue({
  render: h => h(App),
}).$mount('#app')
